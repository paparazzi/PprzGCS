#ifndef CAMERA_SETTINGS_H
#define CAMERA_SETTINGS_H

#include <QtWidgets>
#include "video_recorder.h"

/**
 * The cameras to record (Settings, Recording tab): per camera a USB camera of this computer, or the
 * URL of a stream (rtsp://, udp://, srt://...), and its label. A USB camera that is not plugged in
 * is kept, as not connected. They show in the Video panel (its menu).
 */
class CameraSettings : public QWidget
{
    Q_OBJECT
public:
    explicit CameraSettings(QWidget *parent = nullptr);

    QList<CameraConfig> configs();

private:
    struct Row {
        QWidget* box;
        QComboBox* source;
        QLineEdit* label;
    };

    void addRow(CameraConfig c);
    void fillSources(Row* r, CameraConfig c);

    QVBoxLayout* rows_lay;
    QList<Row*> rows;
};

#endif // CAMERA_SETTINGS_H
