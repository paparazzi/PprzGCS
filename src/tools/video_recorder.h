#ifndef VIDEO_RECORDER_H
#define VIDEO_RECORDER_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include <QMap>

class QProcess;

/// A camera of the settings (video/cameras): a USB camera (its device) or a stream (its URL)
struct CameraConfig {
    QString source;     // /dev/video0, rtsp://..., udp://..., srt://...
    QString name;       // of a USB camera: to find it back when its device changed
    QString label;      // e.g. Pilot view
};

/// A USB camera of this computer
struct CameraDevice {
    QString path;       // /dev/video0
    QString name;       // its card name
};

/**
 * Records the cameras of the settings in var/logs/video, with the audio (AudioRecorder starts and
 * stops it), and shows them in the Video panel.
 *
 * An ffmpeg per camera (a USB camera opens only once): the recording, H.264 for a USB camera
 * (copied as it comes for a stream), a new file every few minutes, named by its local start time
 * (YY_MM_DD__HH_MM_SS__video__<label>.mkv, readable after a crash); and small JPEG frames for the
 * Video panel. ffmpeg runs while recording, or while a panel shows the camera.
 */
class VideoRecorder : public QObject
{
    Q_OBJECT
public:
    static VideoRecorder* get();

    /// the cameras of a profile (see activeProfile())
    static QList<CameraConfig> cameraConfigs(QString profile);
    static void saveCameraConfigs(QList<CameraConfig> cameras, QString profile);
    /// the USB cameras of this computer (their capture devices)
    static QList<CameraDevice> devices();

    void setRecording(bool recording);
    /// the labels of the cameras (for the Video panel)
    QStringList labels();
    /// a panel shows it (ffmpeg runs) or not anymore
    void addViewer(QString label);
    void removeViewer(QString label);

    /// for the status: the cameras that do not work (label: why), and the files written
    QStringList problems();
    QStringList files();

signals:
    void frame(QString label, QImage image);
    void camerasChanged();

private:
    explicit VideoRecorder(QObject* parent = nullptr);
    ~VideoRecorder();

    struct Camera {
        CameraConfig config;
        QProcess* process = nullptr;
        bool writing = false;       // the process records (else only the preview)
        QByteArray buffer;
        QString error;
        QString file_pattern;
        qint64 started = 0;         // ms
        qint64 last_frame = 0;      // ms
        int viewers = 0;
    };

    void loadSettings();
    void update(Camera* c);
    void startProcess(Camera* c);
    void stopProcess(Camera* c);
    void readFrames(Camera* c);
    QString sourcePath(const CameraConfig& c);

    QList<Camera*> cameras;
    bool recording = false;
    QString folder;
    int chunk_minutes = 10;
    QTimer retry_timer;
};

#endif // VIDEO_RECORDER_H
