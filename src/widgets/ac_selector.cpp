#include "ac_selector.h"
#include "ui_ac_selector.h"
#include <iostream>
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"

ACSelector::ACSelector(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Butbut)
{
    ui->setupUi(this);
    connect(ui->ac_id_spinBox, SIGNAL(valueChanged(int)), this, SLOT(changeAC(int)));
}

ACSelector::~ACSelector()
{
    delete ui;
}

void ACSelector::changeAC(int ac_id) {
    emit(DispatcherUi::get()->ac_selected(QString::number(ac_id)));
}
