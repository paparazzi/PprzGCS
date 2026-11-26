#ifndef AUDIO_RECORDER_H
#define AUDIO_RECORDER_H

#include <QObject>
#include <QTimer>
#include <QMap>
#include <QAudioDevice>
#include <QAudioFormat>
#include "audio_status.h"

class QAudioSource;
class QIODevice;
class QMediaDevices;
#include <sndfile.h>     // (SNDFILE: its struct is named differently in each libsndfile version)

/// A microphone of the settings (audio/mics): the device, and a label per channel
struct MicConfig {
    QByteArray device_id;
    QString device_name;    // to find it back when its id changed (e.g. another USB port)
    QStringList labels;
    bool record = true;     // false: kept in the profile, not recorded
};

/**
 * Records the microphones of the settings (e.g. a DJI Mic receiver: one pilot per channel), in
 * the logs folder (var/logs/audio), for the transcription and the logbook after the flight.
 *
 * - a file per microphone, a new one every few minutes (a crash loses seconds, not the file):
 *   YY_MM_DD__HH_MM_SS__audio__<labels>.opus (or .flac), the UTC start time of its first sample
 *   in its comment
 * - RECORD_STATUS each second (and the status signal, for this GCS: Ivy does not send its
 *   messages back), started and stopped by RECORD_CMD as well
 * - a microphone unplugged: a warning, and recorded again when it is back
 * - with --record: from the start, or while an aircraft is connected (audio/record_when); again after a
 *   crash during a recording
 */
class AudioRecorder : public QObject
{
    Q_OBJECT
public:
    static AudioRecorder* get();

    bool isEnabled() {return enabled;}
    bool isRecording() {return recording;}
    QString name() {return recorder_name;}

    /// by the user (status bar, RECORD_CMD)
    void start();
    void stop();

    /// a microphone of the profile (index in micConfigs) recorded or not (its Record checkbox), right away
    void setMicRecorded(int index, bool on);

    /// the microphones of a profile (see activeProfile())
    static QList<MicConfig> micConfigs(QString profile);
    /// what this run records (pprzgcs --record audio|video|all, all by default)
    static QString recordWhat();
    static bool recordsAudio() {return recordWhat() != "video";}
    static bool recordsVideo() {return recordWhat() != "audio";}
    static void saveMicConfigs(QList<MicConfig> mics, QString profile);
    /// the device of a microphone: by its id, else by its name (null if not connected)
    static QAudioDevice findDevice(const MicConfig& mic);
    /// the format asked of a device: 16 bits at 48 kHz (Opus), its channels
    static QAudioFormat captureFormat(const QAudioDevice& device);
    /// the peak of each channel of samples (dBFS)
    static QList<float> peaks(const char* data, qint64 frames, const QAudioFormat& format);

signals:
    void status(AudioStatus s);

private:
    explicit AudioRecorder(QObject* parent = nullptr);
    ~AudioRecorder();

    struct Mic {
        MicConfig config;
        QAudioSource* source = nullptr;
        QIODevice* io = nullptr;
        QAudioFormat format;
        QByteArray pending;         // the bytes of an incomplete frame
        SNDFILE* sf = nullptr;
        QString file;
        double t0 = 0;              // UTC time of the first sample of this stream
        qint64 frames_total = 0;    // since t0
        qint64 frames_file = 0;     // in the current file
        QList<float> levels;        // peak since the last status
        QString error;
    };

    void doStart();
    void doStop();
    void loadSettings();
    void openMic(Mic* m);
    void closeMic(Mic* m);
    void openFile(Mic* m, double start_time);
    void closeFile(Mic* m);
    void onData(Mic* m);
    void onDevicesChanged();
    void sendStatus();
    /// audio/record_when "aircraft": started when an aircraft is connected, stopped some time after the last one
    bool autoAircraft();
    void checkAircraft();

    bool enabled = false;
    bool recording = false;
    double start_time = 0;
    QString recorder_name;
    QString folder;
    QString format;             // opus, flac
    int chunk_minutes = 10;
    QList<Mic*> mics;

    QTimer status_timer;
    QMediaDevices* media_devices;

    QMap<QString, qint64> aircraft_rx;  // per aircraft: the time of its last telemetry (ms)
    bool auto_started = false;          // started for the aircraft: stopped when they are gone
    qint64 server_lost = 0;             // since when the server does not answer (ms), 0 if it does
    bool user_stopped = false;          // stopped by the user while aircraft are connected: not again until they are gone
};

#endif // AUDIO_RECORDER_H
