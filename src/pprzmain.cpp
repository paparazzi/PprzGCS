#include "pprzmain.h"

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
}
