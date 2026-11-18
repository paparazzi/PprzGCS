#ifndef AUDIO_STATUS_H
#define AUDIO_STATUS_H

#include <QString>
#include <QStringList>
#include <QList>

/// The state of an audio recorder: of this GCS (AudioRecorder), or of another one on the bus
/// (AUDIO_RECORD_STATUS)
struct AudioStatus {
    enum State {IDLE = 0, RECORDING = 1, ERROR = 2};

    QString name;
    int state = IDLE;
    double start_time = 0;      // UTC unix time of the start of the recording, 0 when idle
    QStringList labels;         // per channel
    QList<float> levels;        // per channel: peak since the last status (dBFS)
    QString file;
    QString info;               // error or warning, empty if none
};

#endif // AUDIO_STATUS_H
