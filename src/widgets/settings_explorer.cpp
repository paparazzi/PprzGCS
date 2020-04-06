#include "settings_explorer.h"
#include "ui_settings_explorer.h"
#include "dispatcher_ui.h"
#include "AircraftManager.h"
#include <QPainter>
#include <QPaintEvent>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QDebug>

SettingsExplorer::SettingsExplorer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsExplorer)
{
    ui->setupUi(this);
    //ui->menu_list->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    QList<int> sizes;
    sizes.append(50);
    sizes.append(300);
    ui->splitter->setSizes(sizes);

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &SettingsExplorer::handleNewAC);
    connect(ui->aircraft_combo, qOverload<int>(&QComboBox::currentIndexChanged),
        [=](int ix) {
            (void)ix;
            this->repaint();
        });

    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this,
        [=](QString ac_id) {
        for(auto ids: ac_ids) {
            if(ids.second == ac_id) {
                if(ui->aircraft_combo->currentIndex() != ids.first) {
                    ui->aircraft_combo->setCurrentIndex(ids.first);
                    updateMenu();
                }
            }
        }
    });
}

SettingsExplorer::~SettingsExplorer()
{
    delete ui;
}

void SettingsExplorer::handleNewAC(QString ac_id) {
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    ui->aircraft_combo->addItem(ac.name());
    ac_ids[ui->aircraft_combo->count()-1] = ac_id;
    path[ac_id].push_back(0);
    //path[ac_id].push_back(0);
    updateMenu();
}

void SettingsExplorer::handleRowChanged(int row) {
    (void)row;
    qDebug() << row << "  " << ui->menu_list->count();
    updateMenu();
}

void SettingsExplorer::updateMenu() {
    auto current_ac_id = ac_ids[ui->aircraft_combo->currentIndex()];
    auto settings = AircraftManager::get()->getAircraft(current_ac_id).getSettingMenu();
    assert(path[current_ac_id].size() > 0);
    for(size_t i=0; i < path[current_ac_id].size()-1; i++) {
        auto ix = path[current_ac_id][i];
        settings = settings->getSettingMenus()[ix];
    }


    if(lastMenu != settings) {
        disconnect(ui->menu_list, &QListWidget::currentRowChanged, this, &SettingsExplorer::handleRowChanged);

        QListWidgetItem* item;
        while ((item = ui->menu_list->takeItem(0)) != nullptr)
        {
            delete item;
        }

        //fill left pane
        for(auto sm: settings->getSettingMenus()) {
            ui->menu_list->addItem(sm->getName().c_str());
        }
        ui->menu_list->setCurrentRow(0);


        connect(ui->menu_list, &QListWidget::currentRowChanged, this, &SettingsExplorer::handleRowChanged);
    }


    QLayoutItem* litem;
    while ((litem = ui->settings_area->layout()->takeAt(0)) != nullptr)
    {
        delete litem->widget();
        delete litem;
    }

    auto ssm = settings->getSettingMenus()[static_cast<size_t>(ui->menu_list->currentRow())];
    qDebug() << ssm->getName().c_str();
    for(auto sm: ssm->getSettingMenus()) {
        QPushButton* sbut = new QPushButton(sm->getName().c_str());
        ui->settings_area->layout()->addWidget(sbut);
    }

    for(auto st: ssm->getSettings()) {
        qDebug() << st->getName().c_str();
        QLabel* label = new QLabel(st->getName().c_str());
        ui->settings_area->layout()->addWidget(label);
    }

    lastMenu = settings;

}

void SettingsExplorer::paintEvent(QPaintEvent *event) {
    if(ui->aircraft_combo->count() > 0) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRect(event->rect());
        p.setPen(Qt::NoPen);

        QColor color = AircraftManager::get()->getAircraft(ac_ids[ui->aircraft_combo->currentIndex()]).getColor();
        int hue = color.hue();
        int sat = color.saturation();
        color.setHsv(hue, static_cast<int>(sat*0.5), 255);

        p.fillPath(path, color);
        p.drawPath(path);
    }

    QWidget::paintEvent(event);
}
