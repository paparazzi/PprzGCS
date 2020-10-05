#ifndef PPRZMAIN_H
#define PPRZMAIN_H

#include <QMainWindow>
#include "gcs.h"

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

private:
    static PprzMain* singleton;
    explicit PprzMain(QWidget *parent = nullptr);

};

#endif // PPRZMAIN_H
