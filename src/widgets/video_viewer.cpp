#include "video_viewer.h"
#include <QPainter>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QNetworkDatagram>
#include <QDomElement>

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

    setSource("udp://:5600");
}

void VideoViewer::configure(QDomElement conf)
{
    if(conf.hasAttribute("source")) {
        setSource(conf.attribute("source"));
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

void VideoViewer::setSource(const QString &src)
{
    delete udp;
    udp = nullptr;
    delete tcp;
    tcp = nullptr;
    reconnect_timer->stop();
    tcp_buffer.clear();
    image = QImage();

    source = QUrl(src);
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

void VideoViewer::showFrame(const QByteArray &jpeg)
{
    if(jpeg.isEmpty()) {
        return;
    }
    QImage img;
    if(img.loadFromData(jpeg, "JPEG")) {
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
        p.drawText(rect(), Qt::AlignCenter, tr("No video\n%1").arg(source.toString()));
    }
}
