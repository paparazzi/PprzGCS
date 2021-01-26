#include "configure.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include <QString>
#include <iostream>
#include <QDir>
#include <QVariant>
#include <QStandardPaths>

void setEmptyProperty(const char* name, QVariant value) {
    if(!qApp->property(name).isValid()) {
        qApp->setProperty(name, value);
    } else {
        qDebug() << "property "<< name << " already exists with value: " << value;
    }
}

void set_default_config() {
    setEmptyProperty("IVY_NAME", "PprzGCS");
    setEmptyProperty("IVY_BUS", "127.255.255.255:2010");
    setEmptyProperty("PPRZLINK_ID", "pprzcontrol");

    setEmptyProperty("PAPARAZZI_HOME", qgetenv("PAPARAZZI_HOME"));
    setEmptyProperty("PAPARAZZI_SRC", qgetenv("PAPARAZZI_SRC"));

    setEmptyProperty("PPRZLINK_MESSAGES", qgetenv("PAPARAZZI_HOME") + "/var/messages.xml");
    setEmptyProperty("PATH_GCS_ICON", qgetenv("PAPARAZZI_HOME") + "/data/pictures/gcs_icons");
    setEmptyProperty("DEFAULT_TILE_PROVIDER", "Google");

    setEmptyProperty("MAP_MOVE_HYSTERESIS", 20);
    setEmptyProperty("WAYPOINTS_SIZE", 8);
    setEmptyProperty("CIRCLE_CREATE_MIN_RADIUS", 1.0);
    setEmptyProperty("CIRCLE_STROKE", 4);
    setEmptyProperty("SIZE_HIGHLIGHT_FACTOR", 1.5);

    setEmptyProperty("ITEM_Z_VALUE_HIGHLIGHTED", 100);
    setEmptyProperty("ITEM_Z_VALUE_UNHIGHLIGHTED", 50);
    setEmptyProperty("NAV_SHAPE_Z_VALUE", 150);
    setEmptyProperty("AIRCRAFT_Z_VALUE", 300);

    setEmptyProperty("MAPITEMS_FONT", 18);
    setEmptyProperty("AIRCRAFTS_SIZE", 40);

    setEmptyProperty("DEFAULT_COLOR", "red");
    setEmptyProperty("PATH_AIRCRAFT_ICON", qApp->property("APP_DATA_PATH").toString() + "/pictures/aircraft_icons");

    setEmptyProperty("TRACK_MAX_CHUNKS", 10);
    setEmptyProperty("TRACK_CHUNCK_SIZE", 20);

    setEmptyProperty("APP_STYLE_FILE", qApp->property("APP_DATA_PATH").toString() + "/conf/default_style.qss");
    setEmptyProperty("APP_LAYOUT_FILE", qApp->property("APP_DATA_PATH").toString() + "/conf/default_layout.xml");

    qApp->setProperty("USER_DATA_PATH", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

    setEmptyProperty("MAP_PATH", qApp->property("USER_DATA_PATH").toString() + "/map");
}

void set_user_config(QTextStream& stream) {
    QString line;
    while (stream.readLineInto(&line)) {
        if(line.startsWith("#") || line == "") {
            continue;
        }

        auto strList = line.split(' ');

        if(strList.size() != 2) {
            qDebug() << "config line illformed: " << line;
            continue;
        }

        auto key = strList[0];
        auto value = strList[1];

        auto prop = qApp->property(key.toLocal8Bit().data());
        if(prop.isValid()) {
           auto type = prop.type();
           auto variant = QVariant(value);
           bool converted = variant.convert(static_cast<int>(type));
           if(converted) {
               qApp->setProperty(key.toLocal8Bit().data(), variant);
               // qDebug() << "property " << key << " set to " << variant;
           } else {
               qDebug() << "property " << key << ": can't convert " << value << " to " << prop.typeName();
           }
        } else {
            qApp->setProperty(key.toLocal8Bit().data(), value);
        }

    }
}

void configure(QString config_file) {

    QFile gcsConfig(config_file);
    if(!gcsConfig.open(QFile::ReadOnly | QFile::Text)) {
       qDebug() << "Fail to open config file " << config_file;
    }



    if(!qEnvironmentVariableIsSet("PAPARAZZI_HOME") ||
       !qEnvironmentVariableIsSet("PAPARAZZI_SRC")) {
        std::cerr << "Set environnements variables PAPARAZZI_HOME, PAPARAZZI_SRC!" << std::endl;
        abort();
    }

    QString PAPARAZZI_HOME = qgetenv("PAPARAZZI_HOME");
    QString PAPARAZZI_SRC = qgetenv("PAPARAZZI_SRC");

#ifdef APP_DATA_PATH
    qApp->setProperty("APP_DATA_PATH", APP_DATA_PATH);
#else
#error "you need to define APP_DATA_PATH!"
#endif





    QTextStream stream(&gcsConfig);

    set_default_config();
    set_user_config(stream);

}
