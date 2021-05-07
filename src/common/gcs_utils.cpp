#include "gcs_utils.h"
#include <QDebug>
#include <QRegExp>
#include <QSettings>

QString user_or_app_path(QString data_path) {
    auto settings = getAppSettings();
    auto path = settings.value("USER_DATA_PATH").toString() + "/" + data_path;
    if(QFile::exists(path)) {
        return path;
    } else {
        return settings.value("APP_DATA_PATH").toString() + "/" + data_path;
    }
}

double parse_coordinate(QString str) {
    (void)str;
    QRegExp decimal_rx("[+-]?([0-9]*[.])?[0-9]+");
    QRegExp sexa_rx("([+-]?[0-9]+) ([0-9]+) ((?:[0-9]*[.])?[0-9]+)");
    if(decimal_rx.exactMatch(str)) {
        return str.toDouble();
    }
    else if(sexa_rx.exactMatch(str)) {
        sexa_rx.indexIn(str);
        auto caps = sexa_rx.capturedTexts();
        if(caps.length() == 4) {
            auto deg = std::stoi(caps[1].toStdString());
            auto min = std::stoi(caps[2].toStdString());
            auto sec = std::stoi(caps[3].toStdString());
            double coor = deg + min/60.0 + sec/3600.0;
            return coor;
        }
    }
    throw std::runtime_error(str.toStdString() + " is not a coordinate");
}

QSettings getAppSettings() {
    return QSettings(qApp->property("SETTINGS_PATH").toString(), QSettings::IniFormat);
}
