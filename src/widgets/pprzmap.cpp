#include "pprzmap.h"
#include "ui_pprzmap.h"
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"
#include <iostream>
#include <QStandardItemModel>
#include <QCheckBox>
#include <QMenu>
#include <QAction>

PprzMap::PprzMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PprzMap)
{
    ui->setupUi(this);
}

void PprzMap::toggleTileProvider(bool trig) {
    QAction* a = qobject_cast<QAction*>(QObject::sender());
    if(a != nullptr) {
        ui->map->toggleTileProvider(a->text(), trig, 10, 0.5);
        ui->map->updateTiles();
    }
}

PprzMap::~PprzMap()
{
    delete ui;
}

