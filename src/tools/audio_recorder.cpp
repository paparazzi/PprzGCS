#include "audio_recorder.h"
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"
#include "gcs_utils.h"
#include "pprzmain.h"
#include "video_recorder.h"
#include <QAudioSource>
#include <QMediaDevices>
#include <QStorageInfo>
#include <QHostInfo>
#include <QDir>
#include <QDateTime>
#include <QRegularExpression>
#include <QDebug>
#include <sndfile.h>
#include <cmath>

namespace {
// Opus: clear speech at 24 kbit/s per channel (about 11 MB per channel and hour). libsndfile sets
// it by a compression level, from about 256 kbit/s per channel (0) down to 6 (1)
constexpr double OPUS_KBPS = 24;
constexpr double OPUS_COMPRESSION = 1.0 - (OPUS_KBPS - 6.0) / 250.0;
constexpr qint64 LOW_DISK = 500LL * 1024 * 1024;

double nowUnix() {
    return QDateTime::currentMSecsSinceEpoch() / 1000.0;
}

QString logsFolder() {
    return appConfig()->value("PAPARAZZI_HOME").toString() + "/var/logs";
}

/// var/logs/audio: next to the server logs, apart from them (and from var/logs/video)
QString defaultFolder() {
    return logsFolder() + "/audio";
}
}

AudioRecorder* AudioRecorder::get() {
    static AudioRecorder* recorder = new AudioRecorder(qApp);
    return recorder;
}

AudioRecorder::AudioRecorder(QObject* parent) : QObject(parent),
    media_devices(new QMediaDevices(this))
{
    connect(media_devices, &QMediaDevices::audioInputsChanged, this, &AudioRecorder::onDevicesChanged);
    connect(&status_timer, &QTimer::timeout, this, &AudioRecorder::sendStatus);
    connect(DispatcherUi::get(), &DispatcherUi::appSettingsChanged, this, [this]() {
        // again with the new settings (new files), as it was
        bool was = recording;
        if(recording) {
            doStop();
        }
        loadSettings();
        if(enabled && was) {
            doStart();
        }
        sendStatus();
    });

    PprzDispatcher::get()->bindOptional("AUDIO_RECORD_CMD", this, [this](QString sender, pprzlink::Message msg) {
        (void)sender;
        QString name;
        uint8_t command = 0;
        msg.getField("recorder", name);
        msg.getField("command", command);
        if(!enabled || (!name.isEmpty() && name != recorder_name)) {
            return;
        }
        if(command == 1) {
            start();
        } else {
            stop();
        }
    });

    loadSettings();
    // pprzgcs --record (e.g. the flight sessions of the Paparazzi Center): until PprzGCS closes
    // (not in the simulations: their sessions do not pass it), or again after a crash during a recording
    if(enabled && (appConfig()->value("AUDIO_RECORD_ARG").toBool() ||
                   getAppSettings().value("audio/was_recording").toBool())) {
        doStart();
    }
    status_timer.start(1000);
}

AudioRecorder::~AudioRecorder() {
    for(auto m: std::as_const(mics)) {
        closeMic(m);
        delete m;
    }
}

QList<MicConfig> AudioRecorder::micConfigs(QString profile) {
    QList<MicConfig> list;
    auto settings = getAppSettings();
    int n = settings.beginReadArray(profileKey("audio/mics", profile));
    for(int i = 0; i < n; i++) {
        settings.setArrayIndex(i);
        MicConfig c;
        c.device_id = settings.value("device_id").toByteArray();
        c.device_name = settings.value("device_name").toString();
        auto labels = settings.value("labels").toString();
        c.labels = labels.isEmpty() ? QStringList() : labels.split(";");
        list.append(c);
    }
    settings.endArray();
    return list;
}

void AudioRecorder::saveMicConfigs(QList<MicConfig> list, QString profile) {
    auto settings = getAppSettings();
    settings.remove(profileKey("audio/mics", profile));
    settings.beginWriteArray(profileKey("audio/mics", profile), list.size());
    for(int i = 0; i < list.size(); i++) {
        settings.setArrayIndex(i);
        settings.setValue("device_id", list[i].device_id);
        settings.setValue("device_name", list[i].device_name);
        settings.setValue("labels", list[i].labels.join(";"));
    }
    settings.endArray();
}

QAudioDevice AudioRecorder::findDevice(const MicConfig& mic) {
    auto inputs = QMediaDevices::audioInputs();
    for(auto &d: inputs) {
        if(d.id() == mic.device_id) {
            return d;
        }
    }
    for(auto &d: inputs) {
        if(d.description() == mic.device_name) {
            return d;
        }
    }
    return QAudioDevice();
}

QAudioFormat AudioRecorder::captureFormat(const QAudioDevice& device) {
    QAudioFormat f;
    f.setSampleRate(48000);
    f.setChannelCount(std::clamp(device.preferredFormat().channelCount(), 1, 8));
    f.setSampleFormat(QAudioFormat::Int16);
    if(!device.isFormatSupported(f)) {
        f.setSampleFormat(QAudioFormat::Float);
    }
    return f;
}

QList<float> AudioRecorder::peaks(const char* data, qint64 frames, const QAudioFormat& format) {
    int ch = format.channelCount();
    QList<float> peak(ch, 0.f);
    for(qint64 i = 0; i < frames; i++) {
        for(int c = 0; c < ch; c++) {
            float v;
            if(format.sampleFormat() == QAudioFormat::Int16) {
                v = std::abs(reinterpret_cast<const int16_t*>(data)[i * ch + c] / 32768.f);
            } else {
                v = std::abs(reinterpret_cast<const float*>(data)[i * ch + c]);
            }
            peak[c] = std::max(peak[c], v);
        }
    }
    QList<float> db;
    for(auto p: peak) {
        db.append(p > 1e-6f ? 20.f * std::log10(p) : -120.f);
    }
    return db;
}

void AudioRecorder::loadSettings() {
    auto settings = getAppSettings();
    // (never while editing flight plans)
    enabled = settings.value("audio/enabled").toBool() && PprzMain::launch_type != FLIGHTPLAN_EDIT;
    recorder_name = settings.value("audio/name").toString().trimmed();
    if(recorder_name.isEmpty()) {
        recorder_name = QHostInfo::localHostName();
    }
    recorder_name.replace(" ", "_");
    folder = settings.value("audio/folder").toString();
    if(folder.isEmpty()) {
        folder = defaultFolder();
    }
    format = settings.value("audio/format").toString() == "flac" ? "flac" : "opus";
    chunk_minutes = std::max(1, settings.value("audio/chunk_minutes", 10).toInt());

    for(auto m: std::as_const(mics)) {
        closeMic(m);
        delete m;
    }
    mics.clear();
    for(auto &c: micConfigs(activeProfile())) {
        auto m = new Mic();
        m->config = c;
        mics.append(m);
    }
}

void AudioRecorder::start() {
    doStart();
}

void AudioRecorder::stop() {
    doStop();
}

void AudioRecorder::doStart() {
    if(!enabled) {
        return;
    }
    if(!recording) {
        recording = true;
        start_time = nowUnix();
        getAppSettings().setValue("audio/was_recording", true);
        QDir().mkpath(folder);
        for(auto m: std::as_const(mics)) {
            openMic(m);
        }
        VideoRecorder::get()->setRecording(true);   // the cameras with the microphones
    }
    sendStatus();
}

void AudioRecorder::doStop() {
    if(recording) {
        recording = false;
        start_time = 0;
        for(auto m: std::as_const(mics)) {
            closeMic(m);
        }
        VideoRecorder::get()->setRecording(false);
    }
    getAppSettings().setValue("audio/was_recording", false);
    sendStatus();
}

void AudioRecorder::openMic(Mic* m) {
    if(m->source != nullptr) {
        return;
    }
    m->error.clear();
    auto device = findDevice(m->config);
    if(device.isNull()) {
        return;     // (not connected: in the status; opened when it is plugged in)
    }
    m->format = captureFormat(device);
    m->source = new QAudioSource(device, m->format, this);
    m->source->setBufferSize(m->format.bytesForDuration(500000));
    connect(m->source, &QAudioSource::stateChanged, this, [=,this](QAudio::State state) {
        if(state == QAudio::StoppedState && m->source != nullptr && m->source->error() != QAudio::NoError) {
            qDebug() << "audio recorder:" << m->config.device_name << "error" << m->source->error();
            closeMic(m);
            m->error = "error";
        }
    });
    m->pending.clear();
    m->t0 = 0;
    m->frames_total = 0;
    m->levels = QList<float>(m->format.channelCount(), -120.f);
    m->io = m->source->start();
    if(m->io == nullptr) {
        closeMic(m);
        m->error = "cannot open";
        return;
    }
    connect(m->io, &QIODevice::readyRead, this, [=,this]() {onData(m);});
}

void AudioRecorder::closeMic(Mic* m) {
    if(m->source != nullptr) {
        auto source = m->source;
        m->source = nullptr;
        m->io = nullptr;
        source->disconnect(this);
        source->stop();
        source->deleteLater();
    }
    closeFile(m);
}

void AudioRecorder::openFile(Mic* m, double t) {
    closeFile(m);
    // the labels in the name: who is on the file (e.g. Pilot+Safety_pilot)
    QString who = m->config.labels.join("+");
    if(who.isEmpty()) {
        who = m->config.device_name;
    }
    who.replace(QRegularExpression("[^A-Za-z0-9+_-]+"), "_");
    auto date = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(t * 1000));
    QString base = folder + "/" + date.toString("yy_MM_dd__HH_mm_ss") + "__audio__" + who;
    QString ext = format == "flac" ? ".flac" : ".opus";
    QString path = base + ext;
    for(int i = 2; QFile::exists(path); i++) {
        path = QString("%1_%2%3").arg(base).arg(i).arg(ext);
    }

    SF_INFO info = {};
    info.samplerate = m->format.sampleRate();
    info.channels = m->format.channelCount();
    info.format = format == "flac" ? (SF_FORMAT_FLAC | SF_FORMAT_PCM_16) : (SF_FORMAT_OGG | SF_FORMAT_OPUS);
    m->sf = sf_open(path.toUtf8().constData(), SFM_WRITE, &info);
    if(m->sf == nullptr) {
        m->error = QString("cannot write %1: %2").arg(path, sf_strerror(nullptr));
        qDebug() << "audio recorder:" << m->error;
        return;
    }
    if(format == "opus") {
        double level = OPUS_COMPRESSION;
        sf_command(m->sf, SFC_SET_COMPRESSION_LEVEL, &level, sizeof(level));
    }
    // when the file starts, and who is on which channel (for the transcription)
    QString comment = QString("start_unix_time=%1 recorder=%2 device=%3 channels=%4")
            .arg(t, 0, 'f', 3).arg(recorder_name, m->config.device_name, m->config.labels.join(";"));
    sf_set_string(m->sf, SF_STR_COMMENT, comment.toUtf8().constData());
    sf_set_string(m->sf, SF_STR_DATE, date.toUTC().toString(Qt::ISODateWithMs).toUtf8().constData());
    sf_set_string(m->sf, SF_STR_SOFTWARE, "PprzGCS");
    sf_set_string(m->sf, SF_STR_TITLE, m->config.labels.join(", ").toUtf8().constData());
    // (as in AUDIO_RECORD_STATUS: relative to var/logs when in it, e.g. audio/26_11_18__...opus)
    auto rel = QDir(logsFolder()).relativeFilePath(path);
    m->file = rel.startsWith("..") ? path : rel;
    m->error.clear();
    m->frames_file = 0;
}

void AudioRecorder::closeFile(Mic* m) {
    if(m->sf != nullptr) {
        sf_close(m->sf);
        m->sf = nullptr;
    }
}

void AudioRecorder::onData(Mic* m) {
    if(m->io == nullptr) {
        return;
    }
    m->pending.append(m->io->readAll());
    int bpf = m->format.bytesPerFrame();
    qint64 frames = m->pending.size() / bpf;
    if(frames == 0) {
        return;
    }
    int rate = m->format.sampleRate();
    if(m->t0 == 0) {
        // the first sample: now, less the samples already there
        m->t0 = nowUnix() - static_cast<double>(frames) / rate;
    }

    // a new file every chunk_minutes (its start time from the samples: no gap, no overlap)
    if(m->sf == nullptr || m->frames_file >= static_cast<qint64>(chunk_minutes) * 60 * rate) {
        openFile(m, m->t0 + static_cast<double>(m->frames_total) / rate);
    }
    if(m->sf != nullptr) {
        if(m->format.sampleFormat() == QAudioFormat::Int16) {
            sf_writef_short(m->sf, reinterpret_cast<const short*>(m->pending.constData()), frames);
        } else {
            sf_writef_float(m->sf, reinterpret_cast<const float*>(m->pending.constData()), frames);
        }
        m->frames_file += frames;
    }
    m->frames_total += frames;

    auto p = peaks(m->pending.constData(), frames, m->format);
    for(int c = 0; c < p.size() && c < m->levels.size(); c++) {
        m->levels[c] = std::max(m->levels[c], p[c]);
    }
    m->pending.remove(0, frames * bpf);
}

void AudioRecorder::onDevicesChanged() {
    if(!recording) {
        return;
    }
    for(auto m: std::as_const(mics)) {
        bool present = !findDevice(m->config).isNull();
        if(m->source != nullptr && !present) {
            closeMic(m);        // unplugged
        } else if(m->source == nullptr && present) {
            openMic(m);         // (back): a new file
        }
    }
    sendStatus();
}

void AudioRecorder::sendStatus() {
    if(!enabled) {
        return;
    }
    AudioStatus s;
    s.name = recorder_name;
    s.start_time = recording ? start_time : 0;

    QStringList missing, files, errors;
    int open = 0;
    for(auto m: std::as_const(mics)) {
        if(m->source != nullptr) {
            open++;
            for(int c = 0; c < m->levels.size(); c++) {
                s.labels.append(c < m->config.labels.size() && !m->config.labels[c].isEmpty() ?
                                    m->config.labels[c] : QString("%1 %2").arg(m->config.device_name).arg(c + 1));
                s.levels.append(m->levels[c]);
                m->levels[c] = -120.f;
            }
            if(m->sf != nullptr) {
                sf_write_sync(m->sf);
                files.append(m->file);
            }
        } else if(recording) {
            QString who = m->config.labels.isEmpty() ? m->config.device_name : m->config.labels.join(", ");
            if(m->error.isEmpty()) {
                missing.append(who);
            } else {
                errors.append(who + ": " + m->error);
            }
        }
        if(m->source != nullptr && !m->error.isEmpty()) {
            errors.append(m->error);
        }
    }

    // the cameras (VideoRecorder): their problems and files with those of the microphones
    auto video = VideoRecorder::get();
    bool has_cameras = !video->labels().isEmpty();
    auto video_problems = video->problems();
    files.append(video->files());
    QStringList info;
    if(!requestedProfile().isEmpty() && activeProfile().isEmpty()) {
        info.append(QString("profile %1 not on this computer: the default profile").arg(requestedProfile()));
    }
    if(recording && mics.isEmpty() && !has_cameras) {
        info.append("no microphone or camera in the settings");
    }
    if(!missing.isEmpty()) {
        info.append("not connected: " + missing.join(", "));
    }
    info.append(errors);
    info.append(video_problems);
    if(recording) {
        QStorageInfo storage(folder);
        if(storage.isValid() && storage.bytesAvailable() < LOW_DISK) {
            info.append(QString("disk almost full (%1 MB)").arg(storage.bytesAvailable() / (1024 * 1024)));
        }
    }
    s.info = info.join(" · ");
    s.file = files.join(";");
    bool video_ok = has_cameras && video_problems.size() < video->labels().size();
    s.state = !recording ? AudioStatus::IDLE : (open > 0 || video_ok) ? AudioStatus::RECORDING : AudioStatus::ERROR;
    emit status(s);

    auto dispatcher = PprzDispatcher::get();
    if(dispatcher->isStarted() && dispatcher->hasMessage("AUDIO_RECORD_STATUS")) {
        pprzlink::Message msg(dispatcher->getDict()->getDefinition("AUDIO_RECORD_STATUS"));
        QStringList labels;
        for(auto l: s.labels) {
            labels.append(l.replace(" ", "_"));
        }
        std::vector<float> levels(s.levels.begin(), s.levels.end());
        if(levels.empty()) {
            levels.push_back(-120.f);   // (an empty array is not sent: the message could not be parsed)
        }
        msg.addField("recorder", PprzDispatcher::ivyString(s.name));
        msg.addField("state", static_cast<uint8_t>(s.state));
        msg.addField("unix_time", s.start_time);
        msg.addField("labels", PprzDispatcher::ivyString(labels.join(";")));
        msg.addField("levels", levels);
        msg.addField("file", PprzDispatcher::ivyString(s.file));
        msg.addField("info", PprzDispatcher::ivyString(s.info));
        dispatcher->sendMessage(msg);
    }
}
