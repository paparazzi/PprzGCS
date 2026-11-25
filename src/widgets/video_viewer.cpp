#include "video_viewer.h"
#include <QPainter>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QNetworkDatagram>
#include <QDomElement>
#include <QContextMenuEvent>
#include <QInputDialog>
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
        if(label == camera) {
            showImage(img);
        }
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
    // the source chosen last (context menu), else the layout's
    setSource(getAppSettings().value("video/source", layout_source).toString());
}

VideoViewer::~VideoViewer()
{
#if defined(AUDIO_RECORDER_ENABLED)
    if(!camera.isEmpty()) {
        VideoRecorder::get()->removeViewer(camera);
    }
#endif
}

void VideoViewer::configure(QDomElement conf)
{
    if(conf.hasAttribute("source")) {
        layout_source = conf.attribute("source");
        if(!getAppSettings().contains("video/source")) {
            setSource(layout_source);
        }
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
    // the source: the stream of the layout, a camera of the Recording settings, or another stream
    menu.addSection(tr("Source"));
    auto add_source = [&](QString text, QString src) {
        auto action = menu.addAction(text, this, [this, src]() { chooseSource(src); });
        action->setCheckable(true);
        action->setChecked(source_text == src);
    };
    add_source(layout_source, layout_source);
#if defined(AUDIO_RECORDER_ENABLED)
    for(auto &label: VideoRecorder::get()->labels()) {
        add_source(tr("Camera: %1").arg(label), "camera:" + label);
    }
    if(VideoRecorder::get()->labels().isEmpty()) {
        menu.addAction(tr("Cameras: Settings, Recording"))->setEnabled(false);
    }
#endif
    menu.addAction(tr("Video source..."), this, &VideoViewer::askSource);
    menu.addSeparator();
    menu.addAction(tr("Cameras and microphones..."), this, [this]() {
        auto se = new SettingsEditor(false, window());
        se->showTab("Devices");
        se->open();
    });
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

/// Ask for the source; saved for the next start, empty: back to the one of the layout
void VideoViewer::askSource()
{
    QString text = source_text;
    QString error;
    while(true) {
        bool ok;
        text = QInputDialog::getText(this, tr("Video source"),
            error + tr("udp://:5600  receive JPEG frames on UDP port 5600\n"
                       "tcp://192.168.1.10:5601  connect to the TCP server of the video pipeline\n"
                       "Empty: the source of the layout (%1)").arg(layout_source),
            QLineEdit::Normal, text, &ok).trimmed();
        if(!ok) {
            return;
        }
        if(text.isEmpty()) {
            getAppSettings().remove("video/source");
            setSource(layout_source);
            return;
        }
        auto scheme = QUrl(text).scheme();
        if(scheme == "udp" || scheme == "tcp") {
            getAppSettings().setValue("video/source", text);
            setSource(text);
            return;
        }
        error = tr("Not a udp:// or tcp:// address.\n\n");
    }
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
    if(!camera.isEmpty()) {
        VideoRecorder::get()->removeViewer(camera);
    }
#endif
    camera.clear();

    source_text = src;
    source = QUrl(src);
    if(src.startsWith("camera:")) {
#if defined(AUDIO_RECORDER_ENABLED)
        camera = src.mid(7);
        VideoRecorder::get()->addViewer(camera);
#else
        qWarning() << "video: built without the recorder, no camera" << src;
#endif
    } else if(source.scheme() == "udp") {
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
        qWarning() << "video: unknown source" << src << "(use udp://:port or tcp://host:port)";
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
    if(src == layout_source) {
        getAppSettings().remove("video/source");
    } else {
        getAppSettings().setValue("video/source", src);
    }
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
        p.drawText(rect(), Qt::AlignCenter, tr("No video\n%1").arg(source_text));
    }
}
