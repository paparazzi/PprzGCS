#include "audio_recorder_status.h"
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"
#include "app_settings.h"
#include "gcs_utils.h"
#include <cmath>
#if defined(AUDIO_RECORDER_ENABLED)
#include "audio_recorder.h"
#endif

namespace {
const char* RED = "#b42318";
const char* AMBER = "#b54708";
const char* GREY = "#8a8f98";

constexpr qint64 STALE_MS = 3000;       // a recorder that stopped reporting
constexpr float NO_SIGNAL_DB = -80;     // below: no signal (e.g. a wireless mic off or out of range),
constexpr qint64 NO_SIGNAL_MS = 5000;   // for that long (a quiet pilot is still above it)

QString duration(qint64 s) {
    if(s < 0) {
        s = 0;
    }
    if(s >= 3600) {
        return QString("%1:%2:%3").arg(s / 3600).arg((s / 60) % 60, 2, 10, QChar('0')).arg(s % 60, 2, 10, QChar('0'));
    }
    return QString("%1:%2").arg(s / 60, 2, 10, QChar('0')).arg(s % 60, 2, 10, QChar('0'));
}
}

AudioLevelBar::AudioLevelBar(QWidget *parent) : QWidget(parent)
{
    setFixedSize(sizeHint());
}

void AudioLevelBar::setLevel(float db, bool no_signal) {
    if(db != this->db || no_signal != this->no_signal) {
        this->db = db;
        this->no_signal = no_signal;
        update();
    }
}

void AudioLevelBar::paintEvent(QPaintEvent* e) {
    (void)e;
    QPainter p(this);
    auto r = rect();
    p.fillRect(r, palette().color(QPalette::Mid));
    if(no_signal) {
        p.setPen(QPen(QColor(AMBER), 2));
        p.drawRect(r.adjusted(1, 1, -1, -1));
        return;
    }
    // -60 dBFS to 0
    double f = std::clamp((db + 60.0) / 60.0, 0.0, 1.0);
    int h = static_cast<int>(std::round(f * r.height()));
    QColor c = db >= -3 ? QColor(RED) : db >= -12 ? QColor(AMBER) : QColor("#1f7a3f");
    p.fillRect(QRect(r.left(), r.bottom() - h + 1, r.width(), h), c);
}

AudioRecorderStatus::AudioRecorderStatus(QWidget *parent) : QWidget(parent)
{
    lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 8, 0);
    lay->setSpacing(10);

    auto dispatcher = PprzDispatcher::get();
    // the recorders of the other GCSs, or programs (Ivy does not send this GCS its own messages)
    dispatcher->bindOptional("AUDIO_RECORD_STATUS", this, [=,this](QString sender, pprzlink::Message msg) {
        (void)sender;
        AudioStatus s;
        uint8_t state = 0;
        std::vector<float> levels;
        QString labels;
        msg.getField("recorder", s.name);
        msg.getField("state", state);
        msg.getField("unix_time", s.start_time);
        msg.getField("labels", labels);
        msg.getField("levels", levels);
        msg.getField("file", s.file);
        msg.getField("info", s.info);
        s.state = state;
        for(auto l: levels) {
            s.levels.append(std::isfinite(l) ? l : -120.f);
        }
        // (spaces in a label sent as _; no label: no channel, its level only fills the message)
        s.labels = labels.isEmpty() ? QStringList() : labels.replace("_", " ").split(";");
        if(s.labels.isEmpty()) {
            s.levels.clear();
        }
        onStatus(s, false);
    });
#if defined(AUDIO_RECORDER_ENABLED)
    connect(AudioRecorder::get(), &AudioRecorder::status, this, [=,this](AudioStatus s) {
        onStatus(s, true);
    });
    connect(DispatcherUi::get(), &DispatcherUi::appSettingsChanged, this, [=,this]() {
        // turned off, or renamed: its old view goes
        for(auto &name: recorders.keys()) {
            auto &r = recorders[name];
            if(r.local && (!AudioRecorder::get()->isEnabled() || name != AudioRecorder::get()->name())) {
                r.box->deleteLater();
                recorders.remove(name);
            }
        }
    });
#endif

    if(dispatcher->hasMessage("LOG_MARK")) {
        mark_button = new QToolButton(this);
        mark_button->setText("Mark");
        mark_button->setAutoRaise(true);
        connect(mark_button, &QToolButton::clicked, this, &AudioRecorderStatus::mark);
        lay->addWidget(mark_button);
        updateMarkKey();
        connect(DispatcherUi::get(), &DispatcherUi::appSettingsChanged, this, &AudioRecorderStatus::updateMarkKey);
    }

    // the time since the start, and recorders that stopped reporting
    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=,this]() {
        for(auto& r: recorders) {
            updateView(r);
        }
    });
    timer->start(1000);
}

void AudioRecorderStatus::updateMarkKey() {
    auto key = QKeySequence(getAppSettings().value("audio/mark_key", "F9").toString());
    QString tip = "Mark this moment in the logs and in the Messages, to find it back in the flight log, "
                  "the audio and the transcript";
    mark_button->setToolTip(key.isEmpty() ? tip : QString("%1 (%2)").arg(tip, key.toString(QKeySequence::NativeText)));
    // anywhere in the window (also with a text field focused: a function key, not a letter)
    delete mark_shortcut;
    mark_shortcut = nullptr;
    if(!key.isEmpty()) {
        mark_shortcut = new QShortcut(key, this);
        mark_shortcut->setContext(Qt::ApplicationShortcut);
        connect(mark_shortcut, &QShortcut::activated, this, &AudioRecorderStatus::mark);
    }
}

void AudioRecorderStatus::onStatus(AudioStatus s, bool local) {
    bool is_new = !recorders.contains(s.name);
    auto& r = recorders[s.name];
    r.local = local;
    r.status = s;
    r.last_seen = QDateTime::currentMSecsSinceEpoch();

    while(r.status.labels.size() < r.status.levels.size()) {
        r.status.labels.append(QString("Ch %1").arg(r.status.labels.size() + 1));
    }
    while(r.last_signal.size() < r.status.levels.size()) {
        r.last_signal.append(r.last_seen);
    }
    for(int i = 0; i < r.status.levels.size(); i++) {
        if(r.status.levels[i] > NO_SIGNAL_DB || r.status.state != AudioStatus::RECORDING) {
            r.last_signal[i] = r.last_seen;
        }
    }

    if(is_new) {
        makeView(r);
    }
    updateView(r);
}

void AudioRecorderStatus::makeView(Recorder& r) {
    r.box = new QWidget(this);
    r.box->setCursor(Qt::PointingHandCursor);
    r.box->setProperty("recorder", r.status.name);
    r.box->installEventFilter(this);
    auto h = new QHBoxLayout(r.box);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(3);
    r.dot = new QLabel(r.box);
    r.dot->setFixedSize(10, 10);
    h->addWidget(r.dot);
    h->addSpacing(2);
    r.text = new QLabel(r.box);
    h->addWidget(r.text);
    h->addSpacing(2);
    // before the Mark button
    lay->insertWidget(mark_button ? lay->indexOf(mark_button) : lay->count(), r.box);
}

void AudioRecorderStatus::updateView(Recorder& r) {
    auto now = QDateTime::currentMSecsSinceEpoch();
    auto& s = r.status;
    bool lost = !r.local && now - r.last_seen > STALE_MS;

    // a bar per channel
    auto h = static_cast<QHBoxLayout*>(r.box->layout());
    while(r.bars.size() > s.levels.size()) {
        r.bars.takeLast()->deleteLater();
    }
    while(r.bars.size() < s.levels.size()) {
        auto b = new AudioLevelBar(r.box);
        h->addWidget(b);
        r.bars.append(b);
    }

    QStringList no_signal;
    for(int i = 0; i < s.levels.size(); i++) {
        bool ns = !lost && s.state == AudioStatus::RECORDING && now - r.last_signal[i] > NO_SIGNAL_MS;
        r.bars[i]->setLevel(lost ? -120 : s.levels[i], ns);
        r.bars[i]->setToolTip(QString("%1: %2 dBFS").arg(s.labels[i]).arg(s.levels[i], 0, 'f', 0));
        if(ns) {
            no_signal.append(s.labels[i]);
        }
    }

    QString text, dot_color = GREY, text_style;
    QString warn = QString("color: %1; font-weight: bold;").arg(AMBER);
    QString alarm = QString("color: %1; font-weight: bold;").arg(RED);
    QStringList warnings;
    if(!s.info.isEmpty()) {
        warnings.append(s.info);
    }
    if(!no_signal.isEmpty()) {
        warnings.append("no signal: " + no_signal.join(", "));
    }
    if(lost) {
        text = "Recorder lost";
        dot_color = RED;
        text_style = alarm;
    } else if(s.state == AudioStatus::RECORDING) {
        text = "REC " + duration(static_cast<qint64>(now / 1000.0 - s.start_time));
        dot_color = RED;
        if(!warnings.isEmpty()) {
            text += " · " + warnings.join(" · ");
            text_style = warn;
        }
    } else if(s.state == AudioStatus::ERROR) {
        text = "Not recording";
        if(!warnings.isEmpty()) {
            text += ": " + warnings.join(" · ");
        }
        dot_color = RED;
        text_style = alarm;
    } else {
        text = "Not recording";
        text_style = QString("color: %1;").arg(AMBER);
    }
    if(recorders.size() > 1) {
        text = s.name + ": " + text;
    }
    r.text->setText(text);
    if(r.text->styleSheet() != text_style) {
        r.text->setStyleSheet(text_style);
    }
    auto dot_style = QString("background: %1; border-radius: 5px;").arg(dot_color);
    if(r.dot->styleSheet() != dot_style) {
        r.dot->setStyleSheet(dot_style);
    }

    QStringList tip;
    tip << QString("Audio recorder %1%2").arg(s.name, r.local ? " (this GCS)" : "");
    if(!s.file.isEmpty()) {
        tip << "File: " + s.file.split(";").join(", ");
    }
    tip << "Click: details, start / stop";
    r.box->setToolTip(tip.join("\n"));
}

bool AudioRecorderStatus::eventFilter(QObject* obj, QEvent* event) {
    if(event->type() == QEvent::MouseButtonRelease) {
        auto name = obj->property("recorder");
        if(name.isValid()) {
            showMenu(name.toString());
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void AudioRecorderStatus::showMenu(QString name) {
    if(!recorders.contains(name)) {
        return;
    }
    auto& r = recorders[name];
    auto& s = r.status;
    bool local = r.local;
    QMenu menu(this);
    auto info = [&](QString text) {
        menu.addAction(text)->setEnabled(false);
    };
    info(QString("Recorder: %1%2").arg(s.name, local ? " (this GCS)" : ""));
    if(s.state == AudioStatus::RECORDING && s.start_time > 0) {
        info("Since " + QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(s.start_time * 1000)).toString("HH:mm:ss"));
    }
    for(auto &f: s.file.split(";", Qt::SkipEmptyParts)) {
        info("File: " + f);
    }
    for(int i = 0; i < s.levels.size(); i++) {
        info(QString("%1: %2 dBFS").arg(s.labels[i]).arg(s.levels[i], 0, 'f', 0));
    }
    if(!s.info.isEmpty()) {
        info(s.info);
    }

    menu.addSeparator();
    bool recording = s.state != AudioStatus::IDLE;
    auto dispatcher = PprzDispatcher::get();
    if(local || dispatcher->hasMessage("AUDIO_RECORD_CMD")) {
        auto action = menu.addAction(recording ? "Stop recording" : "Start recording");
        action->setEnabled(local || !dispatcher->isSilent());
        connect(action, &QAction::triggered, this, [=,this]() {
            if(recording) {
                auto answer = QMessageBox::question(this, "Stop recording",
                                                    QString("Stop the audio recording of %1?").arg(name));
                if(answer != QMessageBox::Yes) {
                    return;
                }
            }
#if defined(AUDIO_RECORDER_ENABLED)
            if(local) {
                if(recording) {
                    AudioRecorder::get()->stop();
                } else {
                    AudioRecorder::get()->start();
                }
                return;
            }
#endif
            sendCommand(name, !recording);
        });
    }
    if(local) {
        connect(menu.addAction("Recording settings..."), &QAction::triggered, this, [=,this]() {
            auto se = new SettingsEditor(false, window());
            se->showTab("Recording");
            se->open();
        });
    }
    menu.exec(QCursor::pos());
}

void AudioRecorderStatus::sendCommand(QString recorder, bool start) {
    auto dispatcher = PprzDispatcher::get();
    if(!dispatcher->isStarted()) {
        return;
    }
    pprzlink::Message msg(dispatcher->getDict()->getDefinition("AUDIO_RECORD_CMD"));
    msg.addField("recorder", PprzDispatcher::ivyString(recorder));
    msg.addField("command", static_cast<uint8_t>(start ? 1 : 0));
    dispatcher->sendMessage(msg);
}

void AudioRecorderStatus::mark() {
    auto dispatcher = PprzDispatcher::get();
    auto status_bar = qobject_cast<QStatusBar*>(parentWidget());
    auto now = QDateTime::currentDateTime();
    auto show = [=](QString text) {
        if(status_bar) {
            status_bar->showMessage(text, 5000);
        }
    };
    if(dispatcher->isSilent() || !dispatcher->isStarted()) {
        show("Mark not sent: " + QString(dispatcher->isSilent() ? "silent mode" : "not connected"));
        return;
    }

    pprzlink::Message msg(dispatcher->getDict()->getDefinition("LOG_MARK"));
    msg.addField("unix_time", now.toMSecsSinceEpoch() / 1000.0);
    msg.addField("source", PprzDispatcher::ivyString(dispatcher->getId()));
    msg.addField("ac_id", PprzDispatcher::ivyString(DispatcherUi::get()->getSelectedAcId()));
    msg.addField("text", PprzDispatcher::ivyString(QString()));
    dispatcher->sendMessage(msg);
    emit DispatcherUi::get()->markSent(now.toMSecsSinceEpoch() / 1000.0, DispatcherUi::get()->getSelectedAcId());

    show("Mark set at " + now.toString("HH:mm:ss"));
    if(mark_button) {
        mark_button->setText("Marked");
        mark_button->setStyleSheet(QString("color: %1; font-weight: bold;").arg(RED));
        QTimer::singleShot(1500, mark_button, [=,this]() {
            mark_button->setText("Mark");
            mark_button->setStyleSheet("");
        });
    }
}
