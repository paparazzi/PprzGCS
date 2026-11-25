#include "video_viewer.h"
#include <QPainter>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QNetworkDatagram>
#include <QDomElement>
#include <QContextMenuEvent>
#include <QMenu>
#include <QToolButton>
#include "app_settings.h"
#include "dispatcher_ui.h"
#include "gcs_utils.h"
#if defined(AUDIO_RECORDER_ENABLED)
#include "video_recorder.h"
#include "audio_recorder.h"
#endif

static constexpr int NO_VIDEO_TIMEOUT = 1000;   // ms without frame before "No video"
static constexpr int RECONNECT_PERIOD = 1000;   // ms between TCP connection attempts
static constexpr int MAX_TCP_BUFFER = 32 << 20; // drop data beyond this without a complete frame
static constexpr int MIN_WIDTH = 160;
static constexpr int MAX_WIDTH = 1920;

VideoViewer::VideoViewer(QWidget *parent) : QWidget(parent)
{
    reconnect_timer = new QTimer(this);
    reconnect_timer->setSingleShot(true);
    reconnect_timer->setInterval(RECONNECT_PERIOD);
    connect(reconnect_timer, &QTimer::timeout, this, &VideoViewer::connectTcp);

    // repaint to show "No video" when the frames stop
    watchdog = new QTimer(this);
    watchdog->setInterval(NO_VIDEO_TIMEOUT / 2);
    connect(watchdog, &QTimer::timeout, this, [this]() {
        if(!image.isNull() && last_frame.elapsed() > NO_VIDEO_TIMEOUT) {
            update();
        }
    });
    watchdog->start();

#if defined(AUDIO_RECORDER_ENABLED)
    connect(VideoRecorder::get(), &VideoRecorder::frame, this, [this](QString label, QImage img) {
        if(label == camera && viewing) {
            showImage(img);
        }
    });
    // a stream read here while not recorded: by the recorder while it is (one reader of a UDP port)
    connect(VideoRecorder::get(), &VideoRecorder::recordingChanged, this, [this]() {
        auto rec = VideoRecorder::get();
        bool from_recorder = !VideoRecorder::isJpegStream(rec->source(camera)) || rec->recordedLabels().contains(camera);
        if(!camera.isEmpty() && from_recorder != viewing) {
            setSource(source_text);
        }
    });
    connect(VideoRecorder::get(), &VideoRecorder::camerasChanged, this, [this]() {
        setSource(pickSource());
    });

    // the recording: started and stopped here too (the status bar shows it)
    record_button = new QToolButton(this);
    record_button->setAutoRaise(true);
    record_button->setFocusPolicy(Qt::NoFocus);
    connect(record_button, &QToolButton::clicked, this, []() {
        auto rec = AudioRecorder::get();
        if(rec->isRecording()) {
            rec->stop();
        } else {
            rec->start();
        }
    });
    connect(AudioRecorder::get(), &AudioRecorder::status, this, [this](AudioStatus s) {
        updateRecordButton(true, s.state != AudioStatus::IDLE);
    });
    updateRecordButton(AudioRecorder::get()->isEnabled(), AudioRecorder::get()->isRecording());
    connect(DispatcherUi::get(), &DispatcherUi::appSettingsChanged, this, [this]() {
        // (after the recorder has its new settings)
        QTimer::singleShot(0, this, [this]() {
            updateRecordButton(AudioRecorder::get()->isEnabled(), AudioRecorder::get()->isRecording());
        });
    });
#endif
    setSource(pickSource());
}

VideoViewer::~VideoViewer()
{
#if defined(AUDIO_RECORDER_ENABLED)
    if(viewing) {
        VideoRecorder::get()->removeViewer(camera);
    }
#endif
}

void VideoViewer::configure(QDomElement conf)
{
    if(conf.hasAttribute("source")) {
        layout_source = conf.attribute("source");
        setSource(pickSource());
    }
    if(conf.hasAttribute("width")) {
        preferred_width = qBound(MIN_WIDTH, conf.attribute("width").toInt(), MAX_WIDTH);
    }
}

QSize VideoViewer::sizeHint() const
{
    // height from the aspect ratio of the video (4:3 until a frame is received)
    double aspect = image.isNull() ? 0.75 : double(image.height()) / image.width();
    return QSize(preferred_width, qRound(preferred_width * aspect));
}

QSize VideoViewer::minimumSizeHint() const
{
    return QSize(MIN_WIDTH, MIN_WIDTH * 3 / 4);
}

void VideoViewer::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    // the source: a camera or stream of the Devices settings (else the stream of the layout)
    menu.addSection(tr("Source"));
#if defined(AUDIO_RECORDER_ENABLED)
    auto labels = VideoRecorder::get()->labels();
    for(auto &label: labels) {
        QString src = "camera:" + label;
        auto action = menu.addAction(label, this, [this, src]() { chooseSource(src); });
        action->setCheckable(true);
        action->setChecked(source_text == src);
        action->setToolTip(VideoRecorder::get()->source(label));
    }
    menu.setToolTipsVisible(true);
    if(labels.isEmpty()) {
#endif
        auto action = menu.addAction(tr("%1 (of the layout)").arg(layout_source));
        action->setCheckable(true);
        action->setChecked(true);
        action->setEnabled(false);
#if defined(AUDIO_RECORDER_ENABLED)
    }
    menu.addSeparator();
    menu.addAction(tr("Cameras and streams..."), this, [this]() {
        auto se = new SettingsEditor(false, window());
        se->showTab("Devices");
        se->open();
    });
#endif
    menu.exec(event->globalPos());
}

void VideoViewer::updateRecordButton(bool enabled, bool recording)
{
    if(record_button == nullptr) {
        return;
    }
    // (only with the recorder enabled: Settings, Recording)
    record_button->setVisible(enabled);
    record_button->setText(recording ? tr("\u25cf REC") : tr("\u25cf Record"));
    record_button->setToolTip(recording ? tr("Stop recording the microphones and cameras")
                                        : tr("Record the microphones and cameras"));
    record_button->setStyleSheet(QString("QToolButton {color: %1; background: rgba(0, 0, 0, 120); border-radius: 4px; "
                                         "padding: 2px 6px; font-weight: bold;}")
                                     .arg(recording ? "#ff3b30" : "#dddddd"));
    record_button->adjustSize();
    record_button->move(width() - record_button->width() - 4, 4);
}

void VideoViewer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if(record_button != nullptr) {
        record_button->move(width() - record_button->width() - 4, 4);
    }
}

/// the source: the camera chosen last, else the stream of the layout as a camera of the Devices settings, else
/// the first camera, else the stream of the layout itself (no cameras, or built without the recorder)
QString VideoViewer::pickSource()
{
#if defined(AUDIO_RECORDER_ENABLED)
    auto rec = VideoRecorder::get();
    auto labels = rec->labels();
    auto saved = getAppSettings().value("video/source").toString();
    if(saved.startsWith("camera:") && labels.contains(saved.mid(7))) {
        return saved;
    }
    for(auto &label: labels) {
        if(rec->source(label) == layout_source || rec->source(label) == saved) {
            return "camera:" + label;
        }
    }
    if(!labels.isEmpty()) {
        return "camera:" + labels.first();
    }
#endif
    return layout_source;
}

void VideoViewer::setSource(const QString &src)
{
    delete udp;
    udp = nullptr;
    delete tcp;
    tcp = nullptr;
    reconnect_timer->stop();
    tcp_buffer.clear();
    image = QImage();
#if defined(AUDIO_RECORDER_ENABLED)
    if(viewing) {
        VideoRecorder::get()->removeViewer(camera);
    }
#endif
    camera.clear();

    viewing = false;

    source_text = src;
    QString url = src;
    if(src.startsWith("camera:")) {
#if defined(AUDIO_RECORDER_ENABLED)
        auto rec = VideoRecorder::get();
        camera = src.mid(7);
        url = rec->source(camera);
        if(!VideoRecorder::isJpegStream(url) || rec->recordedLabels().contains(camera)) {
            // its frames from the recorder (ffmpeg): a USB camera or another stream, or recorded
            viewing = true;
            rec->addViewer(camera);
            update();
            return;
        }
        // a JPEG stream not recorded: read here (as fast as it comes, no ffmpeg)
#else
        qWarning() << "video: built without the recorder, no camera" << src;
#endif
    }
    source = QUrl(url);
    if(source.scheme() == "udp") {
        udp = new QUdpSocket(this);
        QHostAddress addr = source.host().isEmpty() ? QHostAddress(QHostAddress::Any) : QHostAddress(source.host());
        if(!udp->bind(addr, source.port(5600), QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
            qWarning() << "video: cannot listen on" << src << udp->errorString();
        }
        connect(udp, &QUdpSocket::readyRead, this, &VideoViewer::readUdp);
    } else if(source.scheme() == "tcp") {
        tcp = new QTcpSocket(this);
        connect(tcp, &QTcpSocket::readyRead, this, &VideoViewer::readTcp);
        connect(tcp, &QTcpSocket::disconnected, reconnect_timer, qOverload<>(&QTimer::start));
        connect(tcp, &QTcpSocket::errorOccurred, reconnect_timer, qOverload<>(&QTimer::start));
        connectTcp();
    } else {
        qWarning() << "video: unknown source" << url << "(use udp://:port or tcp://host:port)";
    }
    update();
}

void VideoViewer::connectTcp()
{
    if(tcp && tcp->state() == QAbstractSocket::UnconnectedState) {
        tcp_buffer.clear();
        tcp->connectToHost(source.host().isEmpty() ? "127.0.0.1" : source.host(), source.port(5601));
    }
}

void VideoViewer::readUdp()
{
    // only show the newest frame
    QByteArray jpeg;
    while(udp->hasPendingDatagrams()) {
        jpeg = udp->receiveDatagram().data();
    }
    showFrame(jpeg);
}

void VideoViewer::readTcp()
{
    tcp_buffer.append(tcp->readAll());

    // frames from the JPEG start (FFD8) to end (FFD9) markers, only show the newest one
    QByteArray jpeg;
    qsizetype pos = 0;
    while(true) {
        qsizetype start = tcp_buffer.indexOf("\xff\xd8", pos);
        if(start < 0) {
            pos = tcp_buffer.size() - 1;
            break;
        }
        qsizetype end = tcp_buffer.indexOf("\xff\xd9", start + 2);
        if(end < 0) {
            pos = start;
            break;
        }
        jpeg = tcp_buffer.mid(start, end + 2 - start);
        pos = end + 2;
    }
    tcp_buffer.remove(0, qMax<qsizetype>(pos, 0));
    if(tcp_buffer.size() > MAX_TCP_BUFFER) {
        tcp_buffer.clear();
    }
    showFrame(jpeg);
}

void VideoViewer::chooseSource(const QString &src)
{
    getAppSettings().setValue("video/source", src);     // kept for the next start
    setSource(src);
}

void VideoViewer::showFrame(const QByteArray &jpeg)
{
    if(jpeg.isEmpty()) {
        return;
    }
    QImage img;
    if(img.loadFromData(jpeg, "JPEG")) {
        showImage(img);
    }
}

void VideoViewer::showImage(const QImage &img)
{
    {
        bool new_aspect = image.isNull() || img.width() * image.height() != img.height() * image.width();
        image = img;
        if(new_aspect) {
            updateGeometry();
        }
        last_frame.start();
        update();
    }
}

void VideoViewer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.fillRect(rect(), Qt::black);

    if(!image.isNull()) {
        QSize s = image.size().scaled(size(), Qt::KeepAspectRatio);
        QRect r(QPoint((width() - s.width()) / 2, (height() - s.height()) / 2), s);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.drawImage(r, image);
    }

    if(image.isNull() || last_frame.elapsed() > NO_VIDEO_TIMEOUT) {
        if(!image.isNull()) {
            p.fillRect(rect(), QColor(0, 0, 0, 160));
        }
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, tr("No video\n%1").arg(camera.isEmpty() ? source_text : camera));
    }
}
