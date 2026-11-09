#ifndef SPEAKER_H
#define SPEAKER_H

#include <QObject>
#include <QProcess>
#if defined(QT_TEXTTOSPEECH)
#include <QTextToSpeech>
#endif
#include "PprzToolbox.h"
#include "PprzApplication.h"
#include "pprz_dispatcher.h"



class Speaker : public PprzTool
{
    Q_OBJECT
public:
    explicit Speaker(PprzApplication* app, PprzToolbox* toolbox);
    void setToolbox(PprzToolbox* toolbox) override;
    void addSentence(QString txt, int priority=0, int validity=30);
    void registerMessage(QString name, QString text, int period, int expire, int priority, bool onChange, QString postprocessing);
    void setLocale(QString l);

    /// whether this system can speak: Qt TextToSpeech, or spd-say or espeak-ng
    static bool available();
signals:

public slots:
    void enableSpeech(bool s);

private:

    struct Sentence {
        QString text;
        int priority;
        QTime expiration;
    };

    struct Message {
        QString name;
        QString text;
        int timeout;
        int expire;
        int priority;
        bool onChange;
        QString postprocessing;
    };


    void bindMessage(struct Message msg);
    void handleMsg(struct Message msg, QString sender, pprzlink::Message pprz_msg);

    template<typename T>
    QString value_to_string(T val);

    QString value_to_string(double val);
    QString value_to_string(float val);
    QString value_to_string(QString val);

    QString postprocess(QString process, QString valstr, QString ac_id);

    template<typename T>
    void make_sentence(struct Message msg, QString field, pprzlink::Message pprz_msg, QString ac_id);

    void say();
    bool speaking();
    void speak(QString text);

    /// the speech program of the system (spd-say or espeak-ng), and its arguments before the text
    static QStringList systemCommand();

#if defined(QT_TEXTTOSPEECH)
    QTextToSpeech* tts;
#endif
    QProcess* process;
    bool enabled;
    QString locale;

    QList<Sentence> sentences;
    QList<Message> messages;
    QList<long> bids;
    QMap<QString, QTime> msg_times;
    QMap<QString, QString> last_values;

    QTimer* timer;

};

#endif // SPEAKER_H
