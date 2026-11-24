#include "video_recorder.h"
#include "dispatcher_ui.h"
#include "gcs_utils.h"
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QRegularExpression>
#include <QDebug>
#if defined(Q_OS_LINUX)
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <linux/videodev2.h>
#endif

namespace {
constexpr int RETRY_MS = 3000;          // a camera that failed (unplugged, stream down): tried again
constexpr qint64 NO_VIDEO_MS = 5000;    // running without a frame for that long: a problem
constexpr int MAX_BUFFER = 8 << 20;

QString logsFolder() {
    return appConfig()->value("PAPARAZZI_HOME").toString() + "/var/logs";
}

QString fileLabel(QString label) {
    return label.replace(QRegularExpression("[^A-Za-z0-9+_-]+"), "_");
}
}

VideoRecorder* VideoRecorder::get() {
    static VideoRecorder* recorder = new VideoRecorder(qApp);
    return recorder;
}

VideoRecorder::VideoRecorder(QObject* parent) : QObject(parent)
{
    retry_timer.setInterval(RETRY_MS);
    connect(&retry_timer, &QTimer::timeout, this, [this]() {
        for(auto c: std::as_const(cameras)) {
            update(c);
        }
    });
    retry_timer.start();
    connect(DispatcherUi::get(), &DispatcherUi::appSettingsChanged, this, [this]() {
        // the viewers stay (by label)
        QMap<QString, int> viewers;
        for(auto c: std::as_const(cameras)) {
            viewers[c->config.label] = c->viewers;
            stopProcess(c);
            delete c;
        }
        cameras.clear();
        loadSettings();
        for(auto c: std::as_const(cameras)) {
            c->viewers = viewers.value(c->config.label, 0);
            update(c);
        }
        emit camerasChanged();
    });
    loadSettings();
}

VideoRecorder::~VideoRecorder() {
    for(auto c: std::as_const(cameras)) {
        stopProcess(c);
        delete c;
    }
}

QList<CameraConfig> VideoRecorder::cameraConfigs(QString profile) {
    QList<CameraConfig> list;
    auto settings = getAppSettings();
    int n = settings.beginReadArray(profileKey("video/cameras", profile));
    for(int i = 0; i < n; i++) {
        settings.setArrayIndex(i);
        CameraConfig c;
        c.source = settings.value("source").toString();
        c.name = settings.value("name").toString();
        c.label = settings.value("label").toString();
        if(c.label.isEmpty()) {
            c.label = QString("Camera %1").arg(i + 1);
        }
        list.append(c);
    }
    settings.endArray();
    return list;
}

void VideoRecorder::saveCameraConfigs(QList<CameraConfig> list, QString profile) {
    auto settings = getAppSettings();
    settings.remove(profileKey("video/cameras", profile));
    settings.beginWriteArray(profileKey("video/cameras", profile), list.size());
    for(int i = 0; i < list.size(); i++) {
        settings.setArrayIndex(i);
        settings.setValue("source", list[i].source);
        settings.setValue("name", list[i].name);
        settings.setValue("label", list[i].label);
    }
    settings.endArray();
}

QList<CameraDevice> VideoRecorder::devices() {
    QList<CameraDevice> list;
#if defined(Q_OS_LINUX)
    // the capture devices (a USB camera has also a metadata device, not for video)
    QDir dir("/dev");
    auto names = dir.entryList({"video*"}, QDir::System);
    std::sort(names.begin(), names.end(), [](const QString& a, const QString& b) {
        return a.mid(5).toInt() < b.mid(5).toInt();
    });
    for(auto &n: names) {
        QString path = "/dev/" + n;
        int fd = ::open(path.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
        if(fd < 0) {
            continue;
        }
        v4l2_capability cap = {};
        if(ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
            uint32_t caps = (cap.capabilities & V4L2_CAP_DEVICE_CAPS) ? cap.device_caps : cap.capabilities;
            if(caps & V4L2_CAP_VIDEO_CAPTURE) {
                list.append({path, QString::fromUtf8(reinterpret_cast<const char*>(cap.card))});
            }
        }
        ::close(fd);
    }
#endif
    return list;
}

void VideoRecorder::loadSettings() {
    auto settings = getAppSettings();
    folder = settings.value("video/folder").toString();
    if(folder.isEmpty()) {
        folder = logsFolder() + "/video";
    }
    chunk_minutes = std::max(1, settings.value("audio/chunk_minutes", 10).toInt());
    for(auto &c: cameraConfigs(activeProfile())) {
        auto cam = new Camera();
        cam->config = c;
        cameras.append(cam);
    }
}

QStringList VideoRecorder::labels() {
    QStringList l;
    for(auto c: std::as_const(cameras)) {
        l.append(c->config.label);
    }
    return l;
}

void VideoRecorder::setRecording(bool r) {
    recording = r;
    if(r) {
        QDir().mkpath(folder);
    }
    for(auto c: std::as_const(cameras)) {
        update(c);
    }
}

void VideoRecorder::addViewer(QString label) {
    for(auto c: std::as_const(cameras)) {
        if(c->config.label == label) {
            c->viewers++;
            update(c);
        }
    }
}

void VideoRecorder::removeViewer(QString label) {
    for(auto c: std::as_const(cameras)) {
        if(c->config.label == label && c->viewers > 0) {
            c->viewers--;
            update(c);
        }
    }
}

QString VideoRecorder::sourcePath(const CameraConfig& c) {
    if(!c.source.startsWith("/dev/")) {
        return c.source;    // a stream
    }
    // the USB camera: by its device, else by its name (another USB port, or plugged after another)
    auto devs = devices();
    for(auto &d: devs) {
        if(d.path == c.source && (c.name.isEmpty() || d.name == c.name)) {
            return d.path;
        }
    }
    for(auto &d: devs) {
        if(!c.name.isEmpty() && d.name == c.name) {
            return d.path;
        }
    }
    return QString();
}

void VideoRecorder::update(Camera* c) {
    bool needed = recording || c->viewers > 0;
    bool running = c->process != nullptr;
    if(!needed) {
        stopProcess(c);
        c->error.clear();
        return;
    }
    if(running && c->writing != recording) {
        stopProcess(c);     // (again, with or without the files)
        running = false;
    }
    if(!running) {
        startProcess(c);
    }
}

void VideoRecorder::startProcess(Camera* c) {
    QString src = sourcePath(c->config);
    if(src.isEmpty()) {
        c->error = "not connected";
        return;
    }
    bool usb = src.startsWith("/dev/");
    QStringList args = {"-hide_banner", "-loglevel", "error", "-nostdin"};
    if(usb) {
        args << "-f" << "v4l2" << "-i" << src;
    } else {
        // a stream: analysed for 1 s (not 5), the file starts with the audio
        args << "-analyzeduration" << "1000000" << "-probesize" << "1000000";
        if(src.startsWith("rtsp")) {
            args << "-rtsp_transport" << "tcp";
        }
        args << "-i" << src;
    }

    c->writing = recording;
    if(recording) {
        // a USB camera in H.264 (raw or MJPEG would be huge), a stream as it comes
        c->file_pattern = "__video__" + fileLabel(c->config.label) + ".mkv";
        args << "-map" << "0:v:0" << "-an";
        if(usb) {
            args << "-c:v" << "libx264" << "-preset" << "veryfast" << "-crf" << "28" << "-pix_fmt" << "yuv420p" << "-g" << "50";
        } else {
            args << "-c:v" << "copy";
        }
        args << "-f" << "segment" << "-segment_time" << QString::number(chunk_minutes * 60)
             << "-reset_timestamps" << "1" << "-strftime" << "1"
             << folder + "/%y_%m_%d__%H_%M_%S" + c->file_pattern;
    }
    // small JPEG frames for the Video panel
    args << "-map" << "0:v:0" << "-an" << "-vf" << "fps=10,scale='min(640,iw)':-2"
         << "-c:v" << "mjpeg" << "-q:v" << "6" << "-f" << "image2pipe" << "pipe:1";

    auto p = new QProcess(this);
    c->process = p;
    c->buffer.clear();
    c->error.clear();
    c->started = QDateTime::currentMSecsSinceEpoch();
    c->last_frame = 0;
#if defined(Q_OS_LINUX)
    // stops with PprzGCS, also after a crash (no ffmpeg left recording)
    p->setChildProcessModifier([]() {
        prctl(PR_SET_PDEATHSIG, SIGTERM);
    });
#endif
    connect(p, &QProcess::readyReadStandardOutput, this, [=,this]() {readFrames(c);});
    connect(p, &QProcess::readyReadStandardError, this, [=]() {
        auto lines = QString::fromUtf8(p->readAllStandardError()).trimmed().split("\n");
        if(!lines.isEmpty() && !lines.last().isEmpty()) {
            c->error = lines.last().trimmed();
            qDebug() << "video" << c->config.label << c->error;
        }
    });
    connect(p, &QProcess::finished, this, [=,this]() {
        // stopped by itself (unplugged, stream down): tried again by the retry timer
        if(c->process == p) {
            c->process = nullptr;
            if(c->error.isEmpty()) {
                c->error = "stopped";
            }
        }
        p->deleteLater();
    });
    p->start("ffmpeg", args);
}

void VideoRecorder::stopProcess(Camera* c) {
    if(c->process == nullptr) {
        return;
    }
    auto p = c->process;
    c->process = nullptr;
    p->disconnect(this);
    // ffmpeg closes its files on SIGTERM
    p->terminate();
    if(!p->waitForFinished(2000)) {
        p->kill();
        p->waitForFinished(1000);
    }
    p->deleteLater();
}

void VideoRecorder::readFrames(Camera* c) {
    if(c->process == nullptr) {
        return;
    }
    c->buffer.append(c->process->readAllStandardOutput());
    // frames from the JPEG start (FFD8) to end (FFD9) markers: the newest one
    QByteArray jpeg;
    qsizetype pos = 0;
    while(true) {
        qsizetype start = c->buffer.indexOf("\xff\xd8", pos);
        if(start < 0) {
            pos = c->buffer.size() - 1;
            break;
        }
        qsizetype end = c->buffer.indexOf("\xff\xd9", start + 2);
        if(end < 0) {
            pos = start;
            break;
        }
        jpeg = c->buffer.mid(start, end + 2 - start);
        pos = end + 2;
    }
    c->buffer.remove(0, qMax<qsizetype>(pos, 0));
    if(c->buffer.size() > MAX_BUFFER) {
        c->buffer.clear();
    }
    if(jpeg.isEmpty()) {
        return;
    }
    c->last_frame = QDateTime::currentMSecsSinceEpoch();
    c->error.clear();
    if(c->viewers > 0) {
        QImage img;
        if(img.loadFromData(jpeg, "JPEG")) {
            emit frame(c->config.label, img);
        }
    }
}

QStringList VideoRecorder::problems() {
    QStringList list;
    auto now = QDateTime::currentMSecsSinceEpoch();
    for(auto c: std::as_const(cameras)) {
        if(!recording) {
            continue;   // (only shown: the Video panel says "No video")
        }
        bool no_video = c->process == nullptr ||
                (now - std::max(c->last_frame, c->started) > NO_VIDEO_MS);
        if(no_video) {
            list.append(QString("camera %1: %2").arg(c->config.label, c->error.isEmpty() ? QString("no video") : c->error));
        }
    }
    return list;
}

QStringList VideoRecorder::files() {
    QStringList list;
    if(!recording) {
        return list;
    }
    QDir dir(folder);
    for(auto c: std::as_const(cameras)) {
        if(c->process == nullptr || !c->writing) {
            continue;
        }
        // the newest file of this camera
        auto f = dir.entryList({"*" + c->file_pattern}, QDir::Files, QDir::Time);
        if(!f.isEmpty()) {
            auto rel = QDir(logsFolder()).relativeFilePath(dir.filePath(f.first()));
            list.append(rel.startsWith("..") ? dir.filePath(f.first()) : rel);
        }
    }
    return list;
}
