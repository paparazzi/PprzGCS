#ifndef PPRZMAIN_H
#define PPRZMAIN_H

#include <QMainWindow>
#include <QtWidgets>

class PprzMain : public QMainWindow
{
    Q_OBJECT

public:

    static PprzMain* get() {
        if(singleton == nullptr) {
            singleton = new PprzMain();
        }
        return singleton;
    }

    void setupUi(int width, int height, QWidget* centralWidget);


private:
    static PprzMain* singleton;
    explicit PprzMain(QWidget *parent = nullptr);

    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

};

#endif // PPRZMAIN_H
