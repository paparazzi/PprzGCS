#include "speaker.h"
#include "pprz_dispatcher.h"
#include "gcs_utils.h"
#include "AircraftManager.h"
#include "dispatcher_ui.h"
#include <QStandardPaths>
#include <QRegularExpression>

Speaker::Speaker(PprzApplication* app, PprzToolbox* toolbox) : PprzTool(app, toolbox),
#if defined(QT_TEXTTOSPEECH)
    tts(nullptr),
#endif
    process(nullptr), enabled(false), locale("en_GB")
{
    timer = new QTimer();
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &Speaker::say);
}

void Speaker::addSentence(QString txt, int priority, int validity) {
    if(!speech()) {
        return;
    }
    sentences.append({txt, priority, QTime::currentTime().addMSecs(validity*1000)});
    say();
}

void Speaker::say() {
    if(!speech()) {
        return;
    }
    if(!enabled) {
        enableSpeech(true);     // switched on in the settings meanwhile
    }

    auto now = QTime::currentTime();

    auto it = QMutableListIterator(sentences);
    while(it.hasNext()) {
        auto next = it.next();
        if(next.expiration < now) {
            //qDebug() << "msg discarded: " << next.text;
            it.remove();
        }
    }


    if(!sentences.isEmpty() && !speaking()) {
        int prio = 0;
        int index = 0;

        // priority first, older first
        for(int i=0; i<sentences.size(); ++i) {
            if(sentences[i].priority > prio) {
                prio = sentences[i].priority;
                index = i;
            }
        }

        auto s = sentences.takeAt(index);
        speak(s.text);
        //qDebug() << "saying: " << s.text;
    }

    if(sentences.size() > 0 && !timer->isActive()) {
        timer->start(100);
    }
}

void Speaker::enableSpeech(bool s) {
    // (whether it speaks: speech(), from the settings or --speech; the messages are always listened to)
    if(enabled || !s) {
        return;
    }
    enabled = true;
#if defined(QT_TEXTTOSPEECH)
    // Qt TextToSpeech when it has an engine, else the program of the system
    tts = new QTextToSpeech(this);
    if(tts->state() == QTextToSpeech::State::Error) {
        qDebug() << "Qt TextToSpeech:" << tts->errorString() << "- using" << systemCommand().value(0, "nothing");
        delete tts;
        tts = nullptr;
    } else {
        setLocale(locale);
        return;
    }
#endif
    if(systemCommand().isEmpty()) {
        qDebug() << "speech: neither spd-say nor espeak-ng found (install speech-dispatcher)";
    }
}

/// spd-say (speech-dispatcher, installed with the Ubuntu desktop) first, then espeak-ng
QStringList Speaker::systemCommand() {
    static const QStringList command = []() {
        for(auto &p: QList<QStringList>{{"spd-say", "--wait"}, {"espeak-ng"}, {"espeak"}}) {
            auto path = QStandardPaths::findExecutable(p[0]);
            if(!path.isEmpty()) {
                return QStringList{path} + p.mid(1);
            }
        }
        return QStringList();
    }();
    return command;
}

bool Speaker::available() {
#if defined(QT_TEXTTOSPEECH)
    if(!QTextToSpeech::availableEngines().isEmpty()) {
        return true;
    }
#endif
    return !systemCommand().isEmpty();
}

bool Speaker::speaking() {
#if defined(QT_TEXTTOSPEECH)
    if(tts) {
        return tts->state() != QTextToSpeech::State::Ready;
    }
#endif
    return process && process->state() != QProcess::NotRunning;
}

void Speaker::speak(QString text) {
#if defined(QT_TEXTTOSPEECH)
    if(tts) {
        tts->setRate(qBound(-1.0, getAppSettings().value("notify/speech_rate", 0).toInt() / 100.0, 1.0));
        tts->say(spoken(text));
        return;
    }
#endif
    auto command = systemCommand();
    if(command.isEmpty()) {
        return;
    }
    if(!process) {
        process = new QProcess(this);
        // its messages not in the terminal of the GCS
        process->setStandardOutputFile(QProcess::nullDevice());
        process->setStandardErrorFile(QProcess::nullDevice());
        // the next sentence once this one is said (spd-say --wait, espeak-ng: until the end)
        connect(process, &QProcess::finished, this, &Speaker::say);
    }
    auto settings = getAppSettings();
    process->setProgram(command[0]);
    // the language of the locale (en_GB: en), known by both
    process->setArguments(systemArguments(text, locale.section('_', 0, 0), settings.value("notify/speech_voice").toString(),
                                          settings.value("notify/speech_rate", 0).toInt()));
    process->start();
}

QString Speaker::spoken(QString text) {
    // as the GCS of Paparazzi always said it ("papa ratsi"): read as English, it is "pa-pa-razzy"
    static const QRegularExpression paparazzi("\\bpaparazzi\\b", QRegularExpression::CaseInsensitiveOption);
    return text.replace(paparazzi, "papa ratsi");
}

QStringList Speaker::systemArguments(QString text, QString language, QString voice, int rate) {
    auto command = systemCommand();
    QStringList args = command.mid(1);
    if(command.value(0).endsWith("spd-say")) {
        auto module = voice.section(':', 0, 0);
        auto name = voice.section(':', 1);
        if(!module.isEmpty() && !name.isEmpty()) {
            args << "-o" << module << "-y" << name;     // (its language with it)
        } else {
            args << "-l" << language;
        }
        args << "-r" << QString::number(qBound(-100, rate, 100));
    } else {
        // espeak-ng: words per minute, 175 by default
        args << "-v" << language << "-s" << QString::number(175 + 175 * qBound(-100, rate, 100) / 200);
    }
    return args << "--" << spoken(text);
}

void Speaker::trySay(QString text, QString voice, int rate) {
    auto command = systemCommand();
    if(command.isEmpty()) {
        return;
    }
    QProcess p;
    p.setProgram(command[0]);
    p.setArguments(systemArguments(text, "en", voice, rate));
    p.setStandardOutputFile(QProcess::nullDevice());
    p.setStandardErrorFile(QProcess::nullDevice());
    p.startDetached();
}

QStringList Speaker::voices(QString language) {
    // spd-say -O: the engines (output modules); spd-say -o <module> -L: "NAME LANGUAGE VARIANT", the name with spaces
    auto run = [](QStringList args) {
        QProcess p;
        p.start(systemCommand().value(0), args);
        if(!p.waitForFinished(3000)) {
            p.kill();
            return QStringList();
        }
        auto lines = QString::fromUtf8(p.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
        return lines.mid(1);    // (the title line)
    };
    QStringList list;
    if(!systemCommand().value(0).endsWith("spd-say")) {
        return list;
    }
    for(auto &m: run({"-O"})) {
        auto module = m.trimmed();
        for(auto &line: run({"-o", module, "-L"})) {
            auto words = line.simplified().split(' ');
            if(words.size() < 3) {
                continue;
            }
            auto variant = words.takeLast();
            auto lang = words.takeLast();
            auto name = words.join(' ');
            // the voices of the language, not each variant of espeak-ng (dozens for each)
            if(variant != "none" || !lang.toLower().startsWith(language.toLower())) {
                continue;
            }
            list << QString("%1:%2|%2 (%1, %3)").arg(module, name, lang);
        }
    }
    return list;
}

void Speaker::setLocale(QString l) {
    locale = l;     // (for the program of the system: its language, at the next sentence)
#if defined(QT_TEXTTOSPEECH)
    if(tts) {
        for(auto &l: tts->availableLocales()) {
            if(l.name() == locale) {
                tts->setLocale(l);
            }
        }

        for(auto &v: tts->availableVoices()) {
            qDebug() << v.name();
        }
    }
#endif
}

void Speaker::registerMessage(QString name, QString text, int period, int expire, int priority, bool onChange, QString postprocessing) {
    struct Message msg = {name, text, period*1000, priority, expire, onChange, postprocessing};
    messages.append(msg);
    bindMessage(msg);   // (said only while the speech is on)
}

void Speaker::bindMessage(struct Message msg) {

    auto def = msg.name.split(":");
    auto msg_class = def[0];

    auto bid = PprzDispatcher::get()->bind(def[1], this,
        [=,this](QString sender, pprzlink::Message pprz_msg){

            QString ac_id;
            if(msg_class == "ground") {
                try {
                    pprz_msg.getField("ac_id", ac_id);
                }  catch (pprzlink::no_such_field& e) {
                    return;
                }
            } else if(msg_class == "telemetry") {
                ac_id = sender;
            }

            QString msg_id = msg.name+ac_id;


            auto now = QTime::currentTime();

            if(msg_times.contains(msg_id)) {
                if(msg_times[msg_id].addMSecs(msg.timeout) < now) {
                    handleMsg(msg, ac_id, pprz_msg);
                    msg_times[msg_id] = now;
                }
            } else {
                handleMsg(msg, ac_id, pprz_msg);
                msg_times[msg_id] = now;
            }

        });

    bids.append(bid);

}

void Speaker::handleMsg(struct Message msg, QString ac_id, pprzlink::Message pprz_msg) {
    auto def = msg.name.split(":");
    auto field = def[2];

    auto t = pprz_msg.getDefinition().getField(field).getType();
    auto bt = t.getBaseType();

    if(!t.isArray()) {
        if(bt == pprzlink::BaseType::FLOAT) {
            make_sentence<float>(msg, field, pprz_msg, ac_id);
        } else if(bt == pprzlink::BaseType::DOUBLE) {
            make_sentence<double>(msg, field, pprz_msg, ac_id);
        } else if(bt == pprzlink::BaseType::UINT8) {
            make_sentence<uint8_t>(msg, field, pprz_msg, ac_id);
        } else if(bt == pprzlink::BaseType::UINT16) {
            make_sentence<uint16_t>(msg, field, pprz_msg, ac_id);
        } else if(bt == pprzlink::BaseType::UINT32) {
            make_sentence<uint32_t>(msg, field, pprz_msg, ac_id);
        } else if(bt == pprzlink::BaseType::INT8) {
            make_sentence<int8_t>(msg, field, pprz_msg, ac_id);
        } else if(bt == pprzlink::BaseType::INT16) {
            make_sentence<int16_t>(msg, field, pprz_msg, ac_id);
        } else if(bt == pprzlink::BaseType::INT32) {
            make_sentence<int32_t>(msg, field, pprz_msg, ac_id);
        } else if(bt == pprzlink::BaseType::STRING) {
            make_sentence<QString>(msg, field, pprz_msg, ac_id);
        } else {
            qDebug() << "type not supported yet";
        }
    } else {
        qDebug() << "arrays not supported yet";
    }
}

template<typename T>
QString Speaker::value_to_string(T val) {
    return QString::number(val);
}

QString Speaker::value_to_string(double val) {
    return QString::number(val, 'f', 0);
}

QString Speaker::value_to_string(float val) {
    return QString::number(val, 'f', 0);
}

QString Speaker::value_to_string(QString val) {
    return  val;
}

QString Speaker::postprocess(QString process, QString valstr, QString ac_id) {
    if(process == "block") {
        auto fp = pprzApp()->toolbox()->aircraftManager()->getAircraft(ac_id)->getFlightPlan();
        auto b = fp->getBlock(valstr.toUInt());
        return b->getName();
    } else {
        qDebug() << "no postprocess " << process;
        return valstr;
    }
}

template<typename T>
void Speaker::make_sentence(struct Message msg, QString field, pprzlink::Message pprz_msg, QString ac_id) {
    T val;
    pprz_msg.getField(field, val);
    auto str_val = value_to_string(val);

    QString msg_id = msg.name+ac_id;

    if(msg.onChange && last_values.contains(msg_id) && last_values[msg_id] == str_val) {
        return;
    }

    if(msg.onChange) {
        last_values[msg_id] = str_val;
    }

    if(msg.postprocessing != "") {
        str_val = postprocess(msg.postprocessing, str_val, ac_id);
    }

    QString text = msg.text.replace("{value}", str_val);
    auto ac_name = pprzApp()->toolbox()->aircraftManager()->getAircraft(ac_id)->name();
    text = text.replace("{AC}", ac_name);
    addSentence(text, msg.priority, msg.expire);
}


void Speaker::setToolbox(PprzToolbox* toolbox) {
    PprzTool::setToolbox(toolbox);
    // the warnings and alarms of the map, before the other messages
    connect(DispatcherUi::get(), &DispatcherUi::alert, this, [=,this](QString sender, QString text, bool alarm) {
        QString name = sender == "ground" ? QString("Server") : sender;
        if(AircraftManager::get()->aircraftExists(sender)) {
            name = AircraftManager::get()->getAircraft(sender)->name();
        }
        addSentence(name + ": " + text, alarm ? 10 : 5, 20);
    });
    //connect(PprzDispatcher::get(), &PprzDispatcher::waypoint_moved, this, &AircraftManager::moveWaypoint);
}
