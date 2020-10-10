#include "pprzmain.h"
#include <QApplication>
#include <iostream>
#include "layout_builder.h"
#include <QNetworkProxy>
#include <QProcessEnvironment>
#include "pprz_dispatcher.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile file(":/style.qss");
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream stream(&file);
    a.setStyleSheet(stream.readAll());


    //QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    //QCoreApplication
    //QCommandLineParser

    if(!qEnvironmentVariableIsSet("PAPARAZZI_HOME") ||
       !qEnvironmentVariableIsSet("PAPARAZZI_SRC")  ||
       !qEnvironmentVariableIsSet("PAPARAZZI_GCS_DATA")) {
        std::cerr << "Set environnements variables PAPARAZZI_HOME, PAPARAZZI_SRC and PAPARAZZI_GCS_DATA!" << std::endl;
        abort();
    }

    QString PAPARAZZI_HOME = qgetenv("PAPARAZZI_HOME");
    QString PAPARAZZI_SRC = qgetenv("PAPARAZZI_SRC");
    QString PAPARAZZI_GCS_DATA = qgetenv("PAPARAZZI_GCS_DATA");


    a.setProperty("IVY_NAME", "QPprzControl");
    a.setProperty("IVY_BUS", "127.255.255.255:2010");
    a.setProperty("PPRZLINK_ID", "pprzcontrol");

    a.setProperty("PPRZLINK_MESSAGES", PAPARAZZI_HOME + "/var/messages.xml");
    a.setProperty("PPRZLINK_MESSAGES", PAPARAZZI_HOME + "/var/messages.xml");
    a.setProperty("PATH_GCS_ICON", PAPARAZZI_HOME + "/data/pictures/gcs_icons");
    a.setProperty("DEFAULT_TILE_PROVIDER", "Google");

    a.setProperty("APP_DATA_PATH", PAPARAZZI_GCS_DATA);
    a.setProperty("MAP_MOVE_HYSTERESIS", 20);
    a.setProperty("WAYPOINTS_SIZE", 8);
    a.setProperty("CIRCLE_CREATE_MIN_RADIUS", 1.0);
    a.setProperty("CIRCLE_STROKE", 4);
    a.setProperty("SIZE_HIGHLIGHT_FACTOR", 1.5);

    a.setProperty("ITEM_Z_VALUE_HIGHLIGHTED", 100);
    a.setProperty("ITEM_Z_VALUE_UNHIGHLIGHTED", 50);
    a.setProperty("NAV_SHAPE_Z_VALUE", 150);
    a.setProperty("AIRCRAFT_Z_VALUE", 300);

    a.setProperty("MAPITEMS_FONT", 18);
    a.setProperty("AIRCRAFTS_SIZE", 40);

    a.setProperty("DEFAULT_COLOR", "red");
    a.setProperty("PATH_AIRCRAFT_ICON", PAPARAZZI_GCS_DATA + "/pictures/aircraft_icons");

    a.setProperty("TRACK_MAX_CHUNKS", 10);
    a.setProperty("TRACK_CHUNCK_SIZE", 20);

    QMainWindow* w = build_layout("://test_gcs_qt.xml");

    PprzDispatcher::get()->start();

    w->show();

    return a.exec();
}

