#include "gcs_utils.h"
#include <QDebug>
#include <QRegExp>

QString user_or_app_path(QString data_path) {
    auto path = qApp->property("USER_DATA_PATH").toString() + "/" + data_path;
    if(QFile::exists(path)) {
        return path;
    } else {
        return qApp->property("APP_DATA_PATH").toString() + "/" + data_path;
    }
}

double parse_coordinate(const char* str) {
    (void)str;
    QRegExp decimal_rx("[+-]?([0-9]*[.])?[0-9]+");
    QRegExp sexa_rx("([+-]?[0-9]+) ([0-9]+) ((?:[0-9]*[.])?[0-9]+)");
    if(decimal_rx.exactMatch(str)) {
        qDebug() << str;
        return std::stod(str);
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

    auto msg = std::string(str) + " is not a coordinate";
    throw std::runtime_error(msg);
}
