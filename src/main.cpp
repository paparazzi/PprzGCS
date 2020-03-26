#include "pprzmain.h"
#include <QApplication>
#include <iostream>
#include "layout_builder.h"
#include <QNetworkProxy>
#include <QProcessEnvironment>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    //QCoreApplication
    //QCommandLineParser

    a.setProperty("IVY_NAME", "QPprzControl");
    a.setProperty("IVY_BUS", "127.255.255.255:2010");
    a.setProperty("PPRZLINK_ID", "pprzcontrol");
    a.setProperty("PPRZLINK_MESSAGES", "/home/fabien/paparazzi/var/messages.xml");

    a.setProperty("PATH_GCS_ICON", "/home/fabien/paparazzi/data/pictures/gcs_icons");

    a.setProperty("APP_DATA_PATH", "/home/fabien/DEV/test_qt/PprzGCS/data");
    a.setProperty("PPRZLINK_PATH", "/home/fabien/DEV/test_qt/pprzlink");
    a.setProperty("MAP_MOVE_HYSTERESIS", 20);
    a.setProperty("WAYPOINTS_SIZE", 8);
    a.setProperty("CIRCLE_CREATE_MIN_RADIUS", 1.0);
    a.setProperty("CIRCLE_STROKE", 4);
    a.setProperty("SIZE_HIGHLIGHT_FACTOR", 1.5);
    a.setProperty("ITEM_Z_VALUE_HIGHLIGHTED", 100);
    a.setProperty("ITEM_Z_VALUE_UNHIGHLIGHTED", 50);
    a.setProperty("MAPITEMS_FONT", 18);
    a.setProperty("AIRCRAFTS_SIZE", 25);

    a.setProperty("DEFAULT_COLOR", "red");
    a.setProperty("DEFAULT_AIRCRAFT_ICON", ":/pictures/aircraft_fixedwing1.svg");

    a.setProperty("TRACK_MAX_CHUNKS", 10);
    a.setProperty("TRACK_CHUNCK_SIZE", 20);

//    QNetworkProxy proxy;
//    proxy.setType(QNetworkProxy::DefaultProxy);
//    proxy.setHostName("http://proxy");
//    proxy.setPort(3128);
//    QNetworkProxy::setApplicationProxy(proxy);

    PprzMain* w = build_layout("://test_gcs_qt.xml");

    w->show();

    return a.exec();
}

