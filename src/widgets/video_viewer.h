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
class QToolButton;

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
 *
 * The sources are the cameras and streams of the Devices settings (camera:<label>), chosen with
 * the right click (kept); the source of the layout only without them. A JPEG stream (udp://,
 * tcp://) is read here, except while it is recorded: then, as a USB camera or another stream,
 * its frames come from the recorder (VideoRecorder, ffmpeg), the only reader of the stream.
 * The button in the corner starts and stops the recording (as the status bar does).
 */
class VideoViewer : public QWidget, public Configurable
{
    Q_OBJECT
public:
    explicit VideoViewer(QWidget *parent = nullptr);
    ~VideoViewer();

    void configure(QDomElement) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setSource(const QString &source);
    QString pickSource();
    void readUdp();
    void readTcp();
    void connectTcp();
    void showFrame(const QByteArray &jpeg);
    void showImage(const QImage &img);
    void chooseSource(const QString &src);
    void updateRecordButton(bool enabled, bool recording);

    QUrl source;
    QString source_text;
    QString layout_source = "udp://:5600";  // used when no source is chosen in the GCS
    QString camera;                         // the camera shown (camera:<label>), else empty
    bool viewing = false;                   // its frames from the recorder (else read here: a JPEG stream)
    QUdpSocket *udp = nullptr;
    QTcpSocket *tcp = nullptr;
    QTimer *reconnect_timer;
    QTimer *watchdog;
    QByteArray tcp_buffer;
    QImage image;
    QElapsedTimer last_frame;
    int preferred_width = 320;
    QToolButton *record_button = nullptr;  // starts and stops the recording (microphones and cameras)
};

#endif // VIDEO_VIEWER_H
