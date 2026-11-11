#ifndef VIDEO_VIEWER_H
#define VIDEO_VIEWER_H

#include <QWidget>
#include <QImage>
#include <QElapsedTimer>
#include <QUrl>
#include "configurable.h"

class QUdpSocket;
class QTcpSocket;
class QTimer;

/**
 * Shows a live video made of JPEG frames, e.g. from a GStreamer pipeline started
 * from the Paparazzi Center. The GCS does no video processing: the pipeline decodes,
 * rotates and scales, and sends JPEG frames to:
 *  - udp://:5600 (default): one JPEG per datagram (frames below 64 kB), e.g.
 *    ... ! jpegenc ! udpsink host=127.0.0.1 port=5600
 *  - tcp://127.0.0.1:5601: a stream of JPEG frames of any size, the widget connects
 *    (and reconnects) to the pipeline, e.g. ... ! jpegenc ! tcpserversink port=5601
 *    (also with multipartmux, the frames are found by their JPEG markers)
 *
 * Layout: <widget name="video"><configure source="udp://:5600" width="320"/></widget>
 * The width (pixels) is the size the widget asks for at the start, the height follows
 * the aspect ratio of the video. To change the size, drag the corner of the panel.
 * Right click: choose another source (saved, it replaces the one of the layout).
 */
class VideoViewer : public QWidget, public Configurable
{
    Q_OBJECT
public:
    explicit VideoViewer(QWidget *parent = nullptr);

    void configure(QDomElement) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void setSource(const QString &source);
    void askSource();
    void readUdp();
    void readTcp();
    void connectTcp();
    void showFrame(const QByteArray &jpeg);

    QUrl source;
    QString layout_source = "udp://:5600";  // used when no source is chosen in the GCS
    QUdpSocket *udp = nullptr;
    QTcpSocket *tcp = nullptr;
    QTimer *reconnect_timer;
    QTimer *watchdog;
    QByteArray tcp_buffer;
    QImage image;
    QElapsedTimer last_frame;
    int preferred_width = 320;
};

#endif // VIDEO_VIEWER_H
