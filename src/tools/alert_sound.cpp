#include "alert_sound.h"
#include "gcs_utils.h"
#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <algorithm>

namespace AlertSound {

namespace {
// the list of outputs, read again after a few seconds (an alarm plays every 4 s)
constexpr qint64 OUTPUTS_CACHE_MS = 5000;
}

QList<Output> outputs() {
    static QList<Output> cache;
    static qint64 read = 0;
    auto now = QDateTime::currentMSecsSinceEpoch();
    if(read != 0 && now - read < OUTPUTS_CACHE_MS) {
        return cache;
    }
    read = now;
    cache.clear();
    if(QStandardPaths::findExecutable("pactl").isEmpty()) {
        return cache;
    }
    // "Name: ..." then "Description: ..." per sink (pactl of Ubuntu 22.04 has no JSON)
    QProcess p;
    p.setProcessEnvironment([]() {
        auto env = QProcessEnvironment::systemEnvironment();
        env.insert("LC_ALL", "C");      // the keys not translated
        return env;
    }());
    p.start("pactl", {"list", "sinks"});
    if(!p.waitForFinished(2000)) {
        p.kill();
        p.waitForFinished(500);
        return cache;
    }
    Output o;
    for(auto &line: QString::fromUtf8(p.readAllStandardOutput()).split('\n')) {
        auto l = line.trimmed();
        if(l.startsWith("Name: ")) {
            o = {l.mid(6).trimmed(), QString()};
        } else if(l.startsWith("Description: ") && !o.name.isEmpty()) {
            o.description = l.mid(13).trimmed();
            cache.append(o);
            o = Output();
        }
    }
    return cache;
}

void play(bool alarm) {
    auto settings = getAppSettings();
    QString output = settings.value("notify/output").toString();
    if(!output.isEmpty()) {
        // not connected (e.g. the Bluetooth intercom is off): heard on the default rather than not at all
        bool found = false;
        for(auto &o: outputs()) {
            found |= o.name == output;
        }
        if(!found) {
            output.clear();
        }
    }
    play(alarm, output, settings.value("notify/volume", 100).toInt());
}

void play(bool alarm, QString output, int volume) {
    volume = std::clamp(volume, 0, 100);
    if(volume == 0) {
        return;
    }
    // paplay first: it plays with PulseAudio and with PipeWire (through pipewire-pulse), while pw-play
    // fails when PipeWire runs next to PulseAudio without its own audio output
    static const QStringList player = []() {
        for(auto &p: QList<QStringList>{{"paplay"}, {"pw-play"}, {"aplay", "-q"}, {"afplay"}}) {
            auto path = QStandardPaths::findExecutable(p[0]);
            if(!path.isEmpty()) {
                return QStringList{path} + p.mid(1);
            }
        }
        return QStringList();
    }();
    QString file = user_or_app_path(alarm ? "sounds/alarm.wav" : "sounds/warning.wav");
    if(player.isEmpty() || !QFile::exists(file)) {
        QApplication::beep();
        return;
    }
    QStringList args = player.mid(1);
    QString name = QFileInfo(player[0]).fileName();
    double v = volume / 100.0;
    if(name == "paplay") {
        // its volume as in the sound settings of the system (100 %: 65536)
        args << QString("--volume=%1").arg(qRound(v * 65536));
        if(!output.isEmpty()) {
            args << "--device=" + output;
        }
    } else if(name == "pw-play") {
        args << "--volume" << QString::number(v * v * v, 'f', 4);  // (linear: the cube of the percentage)
        if(!output.isEmpty()) {
            args << "--target" << output;
        }
    } else if(name == "afplay") {
        args << "-v" << QString::number(v * v * v, 'f', 4);
    }
    // (aplay: no volume, the default output)

    // its messages not in the terminal of the GCS (an alarm plays again every few seconds)
    QProcess p;
    p.setProgram(player[0]);
    p.setArguments(args << file);
    p.setStandardOutputFile(QProcess::nullDevice());
    p.setStandardErrorFile(QProcess::nullDevice());
    p.startDetached();
}

}
