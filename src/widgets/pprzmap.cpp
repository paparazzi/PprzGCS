#include "pprzmap.h"
#include "ui_pprzmap.h"

PprzMap::PprzMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PprzMap)
{
    ui->setupUi(this);
}

PprzMap::~PprzMap()
{
    delete ui;
}
