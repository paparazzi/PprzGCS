#include "gcs_utils.h"
#include <QDebug>
#include <QRegExp>
#include <QSettings>

QString user_or_app_path(QString data_path) {
    auto settings = getAppSettings();
    auto path = appConfig()->value("USER_DATA_PATH").toString() + "/" + data_path;
    if(QFile::exists(path)) {
        return path;
    } else {
        return appConfig()->value("APP_DATA_PATH").toString() + "/" + data_path;
    }
}

double parse_coordinate(QString str) {
    str = str.toUpper();
    QRegExp decimal_rx("[+-]?([0-9]*[.])?[0-9]+");
    QRegExp sexa_rx("([+-]?[0-9]+) ([0-9]+) ((?:[0-9]*[.])?[0-9]+) ?([NSEW]?)");
    if(decimal_rx.exactMatch(str)) {
        return str.toDouble();
    }
    else if(sexa_rx.exactMatch(str)) {
        auto caps = sexa_rx.capturedTexts();
        if(caps.length() == 5) {
            auto deg = caps[1].toInt();
            auto min = caps[2].toInt();
            auto sec = caps[3].toDouble();
            auto dir = caps[4];
            if(dir == "S" || dir == "W") {
                deg = -deg;
                min = -min;
                sec = -sec;
            }
            double coor = deg + min/60.0 + sec/3600.0;
            return coor;
        }
    }
    throw std::runtime_error(str.toStdString() + " is not a coordinate");
}

QSettings getAppSettings() {
    return QSettings(appConfig()->value("SETTINGS_PATH").toString(), QSettings::IniFormat);
}

GlobalConfig* appConfig() {
    return GlobalConfig::get();
}

bool verbose() {
    return qApp->property("VERBOSE").toBool();
}

void setVerbose(bool v) {
    qApp->setProperty("VERBOSE", v);
}

bool speech() {
    return qApp->property("SPEECH").toBool();
}

void setSpeech(bool s) {
    qApp->setProperty("SPEECH", s);
}

void logDebug(QString log_class, QString msg, LogLevel level) {
    (void)level;
    if(qApp->property("VERBOSE").toBool()) {
        auto str = QString("[%1] %2").arg(log_class, msg);
        qDebug() << str;
    }
}
