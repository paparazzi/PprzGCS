#include "flightplan_viewerv2.h"
#include <QVBoxLayout>
#include <QtWidgets>
#include "AircraftManager.h"
#include <QDebug>

FlightPlanViewerV2::FlightPlanViewerV2(QString ac_id, QWidget *parent) : QTabWidget(parent),
    ac_id(ac_id), current_block(0), current_stage(0), labels_stylesheet("")
{
    addTab(make_blocks_tab(), "Blocks");
    addTab(make_waypoints_tab(), "Waypoints");

    if(AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getExeptions().size() > 0) {
        addTab(make_exceptions_tab(), "Exceptions");
    }

    if(AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getVariables().size() > 0) {
        addTab(make_variables_tab(), "Variables");
    }

    if(AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getSectors().size() > 0) {
        addTab(make_sectors_tab(), "Sectors");
    }

    addTab(new QWidget(), "Header");

    connect(AircraftManager::get()->getAircraft(ac_id).getStatus(),
            &AircraftStatus::engine_status, this, &FlightPlanViewerV2::handleNavStatus);
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
        QString icon = block->getIcon().c_str();
        QString txt = block->getText().c_str();
        QString name = block->getName().c_str();

        auto lbl = new QLabel(txt != "" ? txt: name, widget);

        auto go_button = new QToolButton(widget);
        go_button->setText("go");
        connect(go_button, &QPushButton::clicked,
            [=]() {
                auto ACId = ac_id.toStdString();
                pprzlink::Message msg(PprzDispatcher::get()->getDict()->getDefinition("JUMP_TO_BLOCK"));
                msg.addField("ac_id", ACId);
                msg.addField("block_id", block->getNo());
                PprzDispatcher::get()->sendMessage(msg);
            });
        auto child_button = new QToolButton(widget);
        child_button->setText(">");
        lay->addWidget(lbl);
        if(icon != "") {
            QString icon_path = qApp->property("PATH_GCS_ICON").toString() + "/" + icon;
            auto ll = new QLabel(widget);
            ll->setPixmap(QPixmap(icon_path));
            ll->setToolTip(txt);
            lay->addWidget(ll);
        }
        lay->addWidget(go_button);
        lay->addWidget(child_button);
        w_layout->addLayout(lay);

        auto facade = new Facade();
        facade->label = lbl;
        facades.append(facade);


        auto tree= make_tree(block, return_home, facade);
        int index_tree = stack->addWidget(tree);

        connect(go_button, &QToolButton::clicked, [=]() {
            qDebug() << "go to " << name;
        });

        connect(child_button, &QPushButton::clicked, [=]() {
            stack->setCurrentIndex(index_tree);
        });

    }

    if(facades.size() > 0) {
        labels_stylesheet = facades[0]->label->styleSheet();
        buttons_stylesheet = facades[0]->button->styleSheet();
    }

    scroll->setWidget(widget);

    return stack;
}

QWidget* FlightPlanViewerV2::make_tree(shared_ptr<Block> block, std::function<void()> return_home, struct Facade* facade) {
    auto widget = new QWidget(this);
    auto vlay = new QVBoxLayout(widget);
    auto hlay = new QHBoxLayout();
    vlay->addLayout(hlay);

    auto ret_but = new QToolButton(widget);
    ret_but->setText("<");
    auto lbl = new QPushButton(block->getName().c_str(), widget);
    facade->button = lbl;
    hlay->addWidget(ret_but);
    hlay->addWidget(lbl);

    connect(ret_but, &QToolButton::clicked, return_home);
    connect(lbl, &QPushButton::clicked,
        [=]() {
            auto ACId = ac_id.toStdString();
            pprzlink::Message msg(PprzDispatcher::get()->getDict()->getDefinition("JUMP_TO_BLOCK"));
            msg.addField("ac_id", ACId);
            msg.addField("block_id", block->getNo());
            PprzDispatcher::get()->sendMessage(msg);
        });

    auto tree = new QTreeWidget();
    facade->tree = tree;
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
    auto list = new QListWidget(this);

    for(auto waypoint: AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getWaypoints()) {
        QString txt = waypoint->getName().c_str() + QString("\t");

        for(auto att: waypoint->getXmlAttributes()) {
            if(att.first != "name") {
                txt += QString("\t") + att.first.c_str() + "=" + att.second.c_str();
            }
        }

        list->addItem(txt);

    }

    return list;
}

QWidget* FlightPlanViewerV2::make_exceptions_tab() {
    auto list = new QListWidget(this);
    for(auto ex: AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getExeptions()) {
        QString txt = QString("cond: ") + ex->cond.c_str() + QString("\tderoute: ") + ex->deroute.c_str();
        list->addItem(txt);
    }
    return list;
}

QWidget* FlightPlanViewerV2::make_variables_tab() {
    auto list = new QListWidget(this);
    for(auto var: AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getVariables()) {
        QString txt;
        if(var->type == Variable::VARIABLE) {
            txt += QString("var: ");
        } else if(var->type == Variable::ABI_BINDING) {
            txt += QString("abi_binding: ");
        }

        for(auto att:var->attributes) {
            txt += QString("  ") + att.first.c_str() + "=" + att.second.c_str();
        }

        list->addItem(txt);
    }
    return list;
}

QWidget* FlightPlanViewerV2::make_sectors_tab() {
    auto list = new QListWidget(this);
    for(auto sec: AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getSectors()) {
        QString txt;
        if(sec->variant == Sector::SECTOR) {
            txt += QString("sector ") + sec->name.c_str() + ":";
            for(auto corner: sec->corners) {
                txt += QString(" ") + corner.c_str();
            }
        } else if(sec->variant == Sector::KML) {
            txt += QString("kml: ") + sec->kml_file.c_str();
        }

        list->addItem(txt);
    }
    return list;
}

void FlightPlanViewerV2::handleNavStatus() {
    auto msg = AircraftManager::get()->getAircraft(ac_id).getStatus()->getMessage("NAV_STATUS");
    if(msg) {
        uint8_t cur_block, cur_stage;
        //uint32_t block_time, stage_time;
        //float target_lat, target_long, target_climb, target_alt, target_course, dist_to_wp;
        msg->getField("cur_block", cur_block);
        msg->getField("cur_stage", cur_stage);

        QTimer* timer = new QTimer();
        timer->moveToThread(qApp->thread());
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout, this, [=]()
        {
            // main thread
            updateNavStatus(cur_block, cur_stage);
            timer->deleteLater();
        });
        QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));

    }
}

void FlightPlanViewerV2::updateNavStatus(uint8_t cur_block, uint8_t cur_stage) {
    //qDebug() << ac_id << "current block: " << cur_block << "  " << cur_stage;

    if(cur_block != current_block || cur_stage != current_stage) {
        //reset idle state
        facades[current_block]->label->setStyleSheet(labels_stylesheet);
        facades[current_block]->button->setStyleSheet(buttons_stylesheet);
//        for(int i=0; i<facades[current_block]->tree->topLevelItemCount(); i++) {
//            facades[current_block]->tree->topLevelItem(i)->setBackgroundColor(0, Qt::white);
//            facades[current_block]->tree->topLevelItem(i)->setTextColor(0, Qt::black);
//        }
//        for(int i=0; i<facades[cur_block]->tree->topLevelItemCount(); i++) {
//            facades[cur_block]->tree->topLevelItem(i)->setBackgroundColor(0, Qt::white);
//            facades[cur_block]->tree->topLevelItem(i)->setTextColor(0, Qt::black);
//        }


        //set "current" state
        facades[cur_block]->label->setStyleSheet("QLabel { background-color: darkGreen; color: white;}");
        facades[cur_block]->button->setStyleSheet("QPushButton{background-color: darkGreen; color: white;}");
//        assert(facades[cur_block]->tree->topLevelItem(cur_stage) != nullptr);
//        facades[cur_block]->tree->topLevelItem(cur_stage)->setBackgroundColor(0, Qt::darkGreen);
//        facades[cur_block]->tree->topLevelItem(cur_stage)->setTextColor(0, Qt::white);
        current_block = cur_block;
        current_stage = cur_stage;
    }
}
