#ifndef GCS_H
#define GCS_H

#include <QMainWindow>
#include <QWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>

class Gcs
{
public:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *PprzMain, int width, int height, QWidget* centralWidget) {
        centralWidget->setParent(PprzMain);
        PprzMain->resize(width, height);
        menuBar = new QMenuBar(PprzMain);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 555, 22));
        PprzMain->setMenuBar(menuBar);
        mainToolBar = new QToolBar(PprzMain);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        PprzMain->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(PprzMain);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        PprzMain->setStatusBar(statusBar);
        PprzMain->setCentralWidget(centralWidget);
    }
};

#endif // GCS_H
