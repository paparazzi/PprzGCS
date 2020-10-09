#include "flightplan_viewerv2.h"
#include <QVBoxLayout>
#include <QtWidgets>
#include "AircraftManager.h"
#include <QDebug>

FlightPlanViewerV2::FlightPlanViewerV2(QString ac_id, QWidget *parent) : QTabWidget(parent),
    ac_id(ac_id)
{
    addTab(make_blocks_tab(), "blocks");
    addTab(make_waypoints_tab(), "waypoints");
    addTab(new QWidget(), "exceptions");
    addTab(new QWidget(), "header");
}


QWidget* FlightPlanViewerV2::make_blocks_tab() {
    auto stack = new QStackedWidget(this);


    auto scroll = new QScrollArea(stack);
    int index_main = stack->addWidget(scroll);
    scroll->setWidgetResizable(true);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    auto widget = new QWidget();

    auto w_layout = new QVBoxLayout(widget);

    auto return_home = [=](){stack->setCurrentIndex(index_main);};

    for(auto block: AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getBlocks()) {
        auto lay = new QHBoxLayout();
        QString name = block->getName().c_str();

        auto lbl = new QPushButton(name, widget);
        auto child_button = new QToolButton(widget);
        child_button->setText(">");
        lay->addWidget(lbl);
        lay->addWidget(child_button);
        w_layout->addLayout(lay);

        auto tree= make_tree(block, return_home);
        int index_tree = stack->addWidget(tree);

        connect(lbl, &QPushButton::clicked, [=]() {
            qDebug() << name;
        });

        connect(child_button, &QPushButton::clicked, [=]() {
            stack->setCurrentIndex(index_tree);
        });

    }

    scroll->setWidget(widget);

    return stack;
}

QWidget* FlightPlanViewerV2::make_tree(shared_ptr<Block> block, std::function<void()> return_home) {
    auto widget = new QWidget(this);
    auto vlay = new QVBoxLayout(widget);
    auto hlay = new QHBoxLayout();
    vlay->addLayout(hlay);

    auto ret_but = new QToolButton(widget);
    ret_but->setText("<");
    auto lbl = new QLabel(block->getName().c_str(), widget);
    hlay->addWidget(ret_but);
    hlay->addWidget(lbl);

    connect(ret_but, &QToolButton::clicked, return_home);

    auto tree = new QTreeWidget();
    tree->setColumnCount(3);
    tree->setHeaderLabels(QStringList() << "Instr" << "Attrib" << "Value");
    //tree->setHeaderHidden(true);
    for(auto stage: block->getStages()) {
        auto instruction = stage.instruction.c_str();
        auto item = new QTreeWidgetItem(tree);
        item->setText(0, instruction);
        tree->addTopLevelItem(item);
        for(auto [k, v]: stage.attributes) {
            auto attr_item = new QTreeWidgetItem(item);

            QString val = v.c_str();
            val.replace("@DEREF", QString::fromUtf8("\xE2\x86\x92"));
            val.replace("@GT", ">");
            val.replace("@GEQ", QString::fromUtf8("\xE2\x89\xA5"));
            val.replace("@LT", "<");
            val.replace("@LEQ", QString::fromUtf8("\xE2\x89\xA4"));
            val.replace("@AND", "&&");
            val.replace("@OR", "||");

            attr_item->setText(1, k.c_str());
            attr_item->setText(2, val);
            item->addChild(attr_item);
        }
    }
    tree->resizeColumnToContents(2);
    vlay->addWidget(tree);

    return widget;
}

QWidget* FlightPlanViewerV2::make_waypoints_tab() {
    auto scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    auto widget = new QWidget();

    auto w_layout = new QVBoxLayout(widget);

    for(auto waypoint: AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getWaypoints()) {
        QString name = waypoint->getName().c_str();

        auto lbl = new QLabel(name, widget);
        w_layout->addWidget(lbl);
    }

    scroll->setWidget(widget);

    return scroll;
}


