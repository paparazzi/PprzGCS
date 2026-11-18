#ifndef AUDIO_MIC_SETTINGS_H
#define AUDIO_MIC_SETTINGS_H

#include <QtWidgets>
#include <QAudioDevice>
#include <QAudioFormat>
#include "audio_recorder.h"

class QAudioSource;
class QMediaDevices;
class AudioLevelBar;

/**
 * The microphones to record (Settings, Recording tab): per microphone its device and a label per
 * channel, and its level right now (to test it). A microphone that is not plugged in is kept, as
 * not connected.
 */
class AudioMicSettings : public QWidget
{
    Q_OBJECT
public:
    explicit AudioMicSettings(QWidget *parent = nullptr);
    ~AudioMicSettings();

    QList<MicConfig> configs();

protected:
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

private:
    struct Row {
        QWidget* box;
        QComboBox* device;
        QLineEdit* labels;
        QWidget* meters;
        QLabel* channels;
        QList<AudioLevelBar*> bars;
        QAudioSource* source = nullptr;
        QIODevice* io = nullptr;
        QAudioFormat format;
        MicConfig saved;        // (its device when not connected)
    };

    void addRow(MicConfig c);
    void removeRow(Row* r);
    void fillDevices(Row* r);
    void openTest(Row* r);
    void closeTest(Row* r);
    void refreshDevices();

    QVBoxLayout* rows_lay;
    QList<Row*> rows;
    QMediaDevices* media_devices;
};

#endif // AUDIO_MIC_SETTINGS_H
