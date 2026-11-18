#ifndef AUDIO_RECORDER_STATUS_H
#define AUDIO_RECORDER_STATUS_H

#include <QtWidgets>
#include <pprzlinkQt/Message.h>
#include "audio_status.h"

/// The level of an audio channel: a small vertical bar (dBFS)
class AudioLevelBar : public QWidget
{
    Q_OBJECT
public:
    explicit AudioLevelBar(QWidget *parent = nullptr);
    void setLevel(float db, bool no_signal);
    QSize sizeHint() const override {return QSize(6, 14);}

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    float db = -120;
    bool no_signal = false;
};

/**
 * In the status bar: the audio recorders (e.g. the microphones of the pilots): the one of this
 * GCS (AudioRecorder) and the others on the bus (AUDIO_RECORD_STATUS), and a Mark button (also a
 * key, F9 by default) that sends a LOG_MARK.
 * A recorder shows once it reported: recording and since when, the level of each channel, and
 * warnings (a microphone not connected, a channel without signal, a recorder that stopped
 * reporting). A click on a recorder: its details, start / stop, and the recording settings.
 */
class AudioRecorderStatus : public QWidget
{
    Q_OBJECT
public:
    explicit AudioRecorderStatus(QWidget *parent = nullptr);

    /// send a mark (the button, or the key)
    void mark();

private:
    struct Recorder {
        AudioStatus status;
        bool local = false;         // the recorder of this GCS
        QList<qint64> last_signal;  // per channel: last time with a signal (ms)
        qint64 last_seen = 0;       // ms

        QWidget* box = nullptr;
        QLabel* dot = nullptr;
        QLabel* text = nullptr;
        QList<AudioLevelBar*> bars;
    };

    void onStatus(AudioStatus s, bool local);
    void makeView(Recorder& r);
    void updateView(Recorder& r);
    void showMenu(QString name);
    void sendCommand(QString recorder, bool start);
    void updateMarkKey();

    bool eventFilter(QObject* obj, QEvent* event) override;

    QHBoxLayout* lay;
    QToolButton* mark_button = nullptr;
    QShortcut* mark_shortcut = nullptr;
    QMap<QString, Recorder> recorders;
};

#endif // AUDIO_RECORDER_STATUS_H
