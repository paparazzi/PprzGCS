#include "pprzmain.h"
#include <QApplication>
#include <iostream>
#include "layout_builder.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    PprzMain* w = build_layout("://test_gcs_qt.xml");

    w->show();

    return a.exec();
}

