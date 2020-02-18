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

    a.setProperty("APP_DATA_PATH", "/home/fabien/DEV/test_qt/PprzGCS/data");
    a.setProperty("PPRZLINK_PATH", "/home/fabien/DEV/test_qt/pprzlink");
    a.setProperty("MAP_MOVE_HYSTERESIS", 20);

//    QNetworkProxy proxy;
//    proxy.setType(QNetworkProxy::DefaultProxy);
//    proxy.setHostName("http://proxy");
//    proxy.setPort(3128);
//    QNetworkProxy::setApplicationProxy(proxy);

    PprzMain* w = build_layout("://test_gcs_qt.xml");

    w->show();

    return a.exec();
}

