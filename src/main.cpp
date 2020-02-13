#include "pprzmain.h"
#include <QApplication>
#include <iostream>
#include "layout_builder.h"
#include <QNetworkProxy>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setProperty("APP_DATA_PATH", "/home/fabien/DEV/test_qt/PprzGCS/data");

//    QNetworkProxy proxy;
//    proxy.setType(QNetworkProxy::DefaultProxy);
//    proxy.setHostName("http://proxy");
//    proxy.setPort(3128);
//    QNetworkProxy::setApplicationProxy(proxy);

    PprzMain* w = build_layout("://test_gcs_qt.xml");

    w->show();

    return a.exec();
}

