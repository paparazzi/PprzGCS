#include "pprzmain.h"
#include <QMessageBox>
#include <QDebug>


PprzMain* PprzMain::singleton = nullptr;

PprzMain::PprzMain(QWidget *parent) :
    QMainWindow(parent)
{

}

void PprzMain::setupUi(int width, int height, QWidget* centralWidget) {
    centralWidget->setParent(this);
    resize(width, height);
    menuBar = new QMenuBar(this);
    menuBar->setObjectName(QString::fromUtf8("menuBar"));
    menuBar->setGeometry(QRect(0, 0, 555, 22));
    setMenuBar(menuBar);
    mainToolBar = new QToolBar(this);
    mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
    addToolBar(Qt::TopToolBarArea, mainToolBar);
    statusBar = new QStatusBar(this);
    statusBar->setObjectName(QString::fromUtf8("statusBar"));
    setStatusBar(statusBar);
    setCentralWidget(centralWidget);
    setWindowIcon(QIcon(":/pictures/icon.svg"));
    populate_menu();
}

void PprzMain::populate_menu() {

    auto file_menu = menuBar->addMenu("&File");

    auto config_dir = file_menu->addAction("Open config directory");
    connect(config_dir, &QAction::triggered, [=](){
        QString path = QDir::toNativeSeparators(QApplication::applicationDirPath());
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });

    auto quit = file_menu->addAction("&Quit");
    connect(quit, &QAction::triggered, qApp, QApplication::quit);


    auto help_menu = menuBar->addMenu("&Help");
    auto about = help_menu->addAction("&About");

    connect(about, &QAction::triggered, [=](bool checked) {
        (void)checked;

        QMessageBox::about(this,"About PprzGCS",
            "<h1>PprzGCS</h1>"
            "<p>PprzGCS is a ground control software for <a href=\"https://paparazziuav.org\">paparrazi UAV</a> drones.</p>"
            "<p>source code can be found here: "
            "<a href=\"https://github.com/Fabien-B/GcsAgain\">https://github.com/Fabien-B/GcsAgain</a></p>"
            "<p>Note: PprzGCS is a temporary name. Feel fre to make suggestions for a better name!</p>"
            "<p>Copyright (C) 2019-2020 Fabien Bonneval (fabien.bonneval@gmail.com)</p>"
            "<p>PprzGCS is free software; you can redistribute it and/or modify "
            "it under the terms of the <a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">GNU General Public License</a> as published by "
            "the Free Software Foundation; either version 3, or (at your option) "
            "any later version.</p>"
            "<p>The program is provided AS IS with "
            "NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, "
            "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.</p>"
            "<h2>Third-party libraries</h2>"
            "<ul>"
            "<li><a href=\"https://www.qt.io/\">Qt 5.12.5</a></li>"
            "<li><a href=\"https://github.com/enacuavlab/pprzlink\">PprzLink++</a></li>"
            "<li><a href=\"https://proj.org/\">Proj</a></li>"
            "<li><a href=\"https://github.com/leethomason/tinyxml2\">TinyXML-2</a></li>"
            "<li><a href=\"https://libzip.org/\">libzip</a></li>"
            "</ul>"
        );

    });

}
