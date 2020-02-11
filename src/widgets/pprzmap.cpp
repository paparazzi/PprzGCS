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
    //connect(DispatcherUi::get(), SIGNAL(ac_selected(int)), this, SLOT(acChanged(int)));
    auto tileProviders = ui->map->tileProviders();

    ui->tileSourcesButton->setMenu(new QMenu);

    for(auto conf: *tileProviders) {
        QAction* a = new QAction(conf.first);
        a->setCheckable(true);
        ui->tileSourcesButton->menu()->addAction(a);
        connect(a,SIGNAL(toggled(bool)),this,SLOT(toggleTileProvider(bool)));
    }
}

void PprzMap::toggleTileProvider(bool trig) {
    QAction* a = qobject_cast<QAction*>(QObject::sender());
    if(a != nullptr) {
        std::cout << a->text().toStdString() << " : " << trig << std::endl;
        ui->map->toggleTileProvider(a->text(), trig, 10, 0.5);
        ui->map->updateTiles();
    }
}

PprzMap::~PprzMap()
{
    delete ui;
}

