#include "pprzmain.h"
#include "gcs.h"
#include <iostream>
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"

using namespace std;

PprzMain::PprzMain(int width, int height, QWidget* centralWidget, QWidget *parent) :
    QMainWindow(parent),
    ui(new Gcs)
{
    ui->setupUi(this, width, height, centralWidget);
    //connect(ui->pushButton, SIGNAL(clicked()), qApp, SLOT(quit()));
    //connect(ui->select_btn, SIGNAL(clicked()), this, SLOT(select_ac()));
}

PprzMain::~PprzMain()
{
    delete ui;
}


