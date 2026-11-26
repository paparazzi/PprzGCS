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
#include <QTabBar>
#include <QCheckBox>
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
        updateTabs();
        auto rec = VideoRecorder::get();
        bool from_recorder = !VideoRecorder::isJpegStream(rec->source(camera)) || rec->recordedLabels().contains(camera);
        if(!camera.isEmpty() && from_recorder != viewing) {
            setSource(source_text);
        }
    });
    connect(VideoRecorder::get(), &VideoRecorder::camerasChanged, this, [this]() {
        rebuildTabs();
        setSource(pickSource());
    });

    // a tab per camera or stream of the Devices settings: the one shown, and a checkbox to record it
    tabs = new QTabBar(this);
    tabs->setDrawBase(false);
    tabs->setExpanding(false);
    tabs->setDocumentMode(true);
    tabs->setUsesScrollButtons(true);
    tabs->setElideMode(Qt::ElideRight);
    tabs->setFocusPolicy(Qt::NoFocus);
    tabs->setStyleSheet("QTabBar {background: black;} QTabBar::tab {color: #bbbbbb; background: #202020; padding: 2px 6px; border: none; "
                        "margin-right: 1px;} QTabBar::tab:selected {color: white; background: #3a3a3a;}");
    connect(tabs, &QTabBar::currentChanged, this, [this](int i) {
        auto src = "camera:" + tabs->tabData(i).toString();
        if(i >= 0 && src != source_text) {
            chooseSource(src);
        }
    });
    rebuildTabs();

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
        updateTabs();
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
    return QSize(preferred_width, qRound(preferred_width * aspect) + barHeight());
}

QSize VideoViewer::minimumSizeHint() const
{
    return QSize(MIN_WIDTH, MIN_WIDTH * 3 / 4);
}

void VideoViewer::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
#if defined(AUDIO_RECORDER_ENABLED)
    if(VideoRecorder::get()->labels().isEmpty()) {
#endif
        menu.addAction(tr("Source: %1 (of the layout)").arg(layout_source))->setEnabled(false);
#if defined(AUDIO_RECORDER_ENABLED)
    }
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
    layoutBar();
}

void VideoViewer::rebuildTabs()
{
#if defined(AUDIO_RECORDER_ENABLED)
    auto rec = VideoRecorder::get();
    QSignalBlocker block(tabs);
    while(tabs->count() > 0) {
        tabs->removeTab(0);
    }
    for(auto &label: rec->labels()) {
        int i = tabs->addTab(label);
        tabs->setTabData(i, label);
        tabs->setTabToolTip(i, rec->source(label));
        auto check = new QCheckBox(tabs);
        check->setToolTip(tr("Record %1 with the microphones").arg(label));
        check->setFocusPolicy(Qt::NoFocus);
        connect(check, &QCheckBox::toggled, this, [label](bool on) {
            VideoRecorder::get()->setRecordTicked(label, on);
        });
        tabs->setTabButton(i, QTabBar::LeftSide, check);
    }
    tabs->setVisible(tabs->count() > 0);
    updateTabs();
    layoutBar();
    updateGeometry();
#endif
}

void VideoViewer::updateTabs()
{
#if defined(AUDIO_RECORDER_ENABLED)
    // a red dot: recorded now
    static const QIcon red_dot = []() {
        QPixmap pix(12, 12);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#ff3b30"));
        p.drawEllipse(1, 1, 10, 10);
        return QIcon(pix);
    }();
    auto rec = VideoRecorder::get();
    QSignalBlocker block(tabs);
    for(int i = 0; i < tabs->count(); i++) {
        auto label = tabs->tabData(i).toString();
        tabs->setTabIcon(i, rec->isRecorded(label) ? red_dot : QIcon());
        if(auto check = qobject_cast<QCheckBox*>(tabs->tabButton(i, QTabBar::LeftSide))) {
            QSignalBlocker block_check(check);
            check->setChecked(rec->recordTicked(label));
        }
        if(label == camera) {
            tabs->setCurrentIndex(i);
        }
    }
#endif
}

int VideoViewer::barHeight() const
{
    return (tabs != nullptr && tabs->isVisibleTo(this)) ? tabs->sizeHint().height() : 0;
}

QRect VideoViewer::videoRect() const
{
    return rect().adjusted(0, barHeight(), 0, 0);
}

void VideoViewer::layoutBar()
{
    int bar = barHeight();
    int button = (record_button != nullptr && record_button->isVisibleTo(this)) ? record_button->width() : 0;
    if(tabs != nullptr && bar > 0) {
        tabs->setGeometry(0, 0, std::max(0, width() - button - 4), bar);
    }
    if(record_button != nullptr) {
        // at the end of the tabs, else in the corner of the video
        int y = bar > 0 ? std::max(0, (bar - record_button->height()) / 2) : 4;
        record_button->move(width() - record_button->width() - 4, y);
    }
}

void VideoViewer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutBar();
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
        updateTabs();
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

    QRect area = videoRect();
    if(!image.isNull()) {
        QSize s = image.size().scaled(area.size(), Qt::KeepAspectRatio);
        QRect r(QPoint(area.x() + (area.width() - s.width()) / 2, area.y() + (area.height() - s.height()) / 2), s);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.drawImage(r, image);
    }

    if(image.isNull() || last_frame.elapsed() > NO_VIDEO_TIMEOUT) {
        if(!image.isNull()) {
            p.fillRect(area, QColor(0, 0, 0, 160));
        }
        p.setPen(Qt::white);
        p.drawText(area, Qt::AlignCenter, tr("No video\n%1").arg(camera.isEmpty() ? source_text : camera));
    }
}
