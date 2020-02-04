#ifndef PPRZMAIN_H
#define PPRZMAIN_H

#include <QMainWindow>
#include "gcs.h"

class PprzMain : public QMainWindow
{
    Q_OBJECT

public:
    explicit PprzMain(int width, int height, QWidget* centralWidget, QWidget *parent = nullptr);
    ~PprzMain();


private:
    Gcs *ui;

};

#endif // PPRZMAIN_H
