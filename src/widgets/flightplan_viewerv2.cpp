#include "flightplan_viewerv2.h"
#include <QVBoxLayout>
#include <QtWidgets>
#include "AircraftManager.h"
#include <QDebug>
#include <QSettings>
#include "gcs_utils.h"
#include "flightplaneditor.h"

#include "mission_model.h"

FlightPlanViewerV2::FlightPlanViewerV2(QString ac_id, QWidget *parent) : QTabWidget(parent),
    ac_id(ac_id), current_block(255), current_stage(0)
{
    addTab(make_blocks_tab(), "Blocks");
    addTab(new FlightPlanEditor(ac_id, this), "Details");
    addTab(new MissionWidget(ac_id, this), "Missions");


    connect(AircraftManager::get()->getAircraft(ac_id)->getStatus(),
            &AircraftStatus::nav_status, this, &FlightPlanViewerV2::handleNavStatus);
}


/**
 * The blocks: full-width rows (touch friendly). Tapping a row selects it and shows its "Go"
 * button, which jumps to the block. The current block is highlighted, with its time.
 */
QWidget* FlightPlanViewerV2::make_blocks_tab() {
    auto scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto widget = new QWidget();
    widget->setObjectName("blocks");
    // colors from the application style sheet (light and night themes), these when it has none
    if(!qApp->styleSheet().contains("block_row")) widget->setStyleSheet(
        "QPushButton#block_row { border: 0; border-radius: 0; padding: 0 8px; min-height: 40px; background: transparent; text-align: left; }"
        "QLabel#block_detail { color: #57606a; }"
        "QPushButton#block_row:checked { background: #dde7f7; border-left: 4px solid #1d5fc9; }"
        "QPushButton#block_row[current=\"true\"] { background: #cfe9d6; border-left: 4px solid #1f7a3f; }"
        "QPushButton#block_row[current=\"true\"] QLabel { color: #0b3d1c; font-weight: bold; }"
        "QPushButton#block_go { background: #1d5fc9; color: white; font-weight: bold; border: 0; border-radius: 5px; padding: 0 14px; min-height: 30px; }");

    auto vbox = new QVBoxLayout(widget);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);
    auto group = new QButtonGroup(this);
    group->setExclusive(true);

    for(auto block: AircraftManager::get()->getAircraft(ac_id)->getFlightPlan()->getBlocks()) {
        QString icon = block->getIcon();
        QString txt = block->getText();
        QString name = block->getName();

        auto row = new QPushButton(widget);
        row->setObjectName("block_row");
        row->setCheckable(true);
        row->setToolTip(name);
        group->addButton(row);
        auto hbox = new QHBoxLayout(row);
        hbox->setContentsMargins(4, 2, 4, 2);

        auto ll = new QLabel(row);
        ll->setFixedWidth(24);
        if(icon != "") {
            QString icon_path = appConfig()->value("GCS_ICONS_PATH").toString() + "/" + icon;
            ll->setPixmap(QPixmap(icon_path).scaled(22, 22, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        // "Takeoff (wp CLIMB)": the name, then the waypoints in gray
        QString main = txt != "" ? txt: name;
        QString detail;
        static const QRegularExpression re("^(.*\\S)\\s*\\((?:wp\\s+)?(.*)\\)$");
        auto m = re.match(main);
        if(m.hasMatch()) {
            main = m.captured(1);
            detail = m.captured(2);
        }
        auto lbl = new QLabel(main, row);
        auto det = new QLabel(detail, row);
        det->setObjectName("block_detail");
        auto time = new QLabel(row);
        auto go = new QPushButton("Go", row);
        go->setObjectName("block_go");
        go->setToolTip("Jump to " + name);
        go->hide();
        for(auto l: {ll, lbl, det, time}) {
            l->setAttribute(Qt::WA_TransparentForMouseEvents);
        }
        hbox->addWidget(ll);
        hbox->addWidget(lbl);
        hbox->addWidget(det, 1);
        hbox->addWidget(time);
        hbox->addWidget(go);

        connect(row, &QPushButton::toggled, go, &QWidget::setVisible);
        connect(go, &QPushButton::clicked, this,
            [=,this]() {
                pprzlink::Message msg(PprzDispatcher::get()->getDict()->getDefinition("JUMP_TO_BLOCK"));
                msg.addField("ac_id", ac_id);
                msg.addField("block_id", block->getNo());
                PprzDispatcher::get()->sendMessage(msg);
                // done: unselect (the row becomes the current block)
                group->setExclusive(false);
                row->setChecked(false);
                group->setExclusive(true);
            });

        vbox->addWidget(row);
        block_rows.append(row);
        block_times.append(time);
    }
    vbox->addStretch();

    scroll->setWidget(widget);

    return scroll;
}


void FlightPlanViewerV2::handleNavStatus() {
    auto msg = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getMessage("NAV_STATUS");
    if(msg) {
        uint8_t cur_block, cur_stage;
        uint32_t block_time = 0;
        //float target_lat, target_long, target_climb, target_alt, target_course, dist_to_wp;
        msg->getField("cur_block", cur_block);
        msg->getField("cur_stage", cur_stage);
        msg->getField("block_time", block_time);

        QTimer* timer = new QTimer();
        timer->moveToThread(qApp->thread());
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout, this, [=,this]()
        {
            // main thread
            updateNavStatus(cur_block, cur_stage, block_time);
            timer->deleteLater();
        });
        QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));

    }
}

void FlightPlanViewerV2::updateNavStatus(uint8_t cur_block, uint8_t cur_stage, uint32_t block_time) {
    if(cur_block >= block_rows.size()) {
        return;
    }
    if(cur_block != current_block) {
        if(current_block < block_rows.size()) {
            block_rows[current_block]->setProperty("current", false);
            block_rows[current_block]->style()->unpolish(block_rows[current_block]);
            block_rows[current_block]->style()->polish(block_rows[current_block]);
            block_times[current_block]->clear();
            for(auto l: block_rows[current_block]->findChildren<QLabel*>()) {
                l->style()->unpolish(l);
                l->style()->polish(l);
            }
        }
        block_rows[cur_block]->setProperty("current", true);
        block_rows[cur_block]->style()->unpolish(block_rows[cur_block]);
        block_rows[cur_block]->style()->polish(block_rows[cur_block]);
        for(auto l: block_rows[cur_block]->findChildren<QLabel*>()) {
            l->style()->unpolish(l);
            l->style()->polish(l);
        }
    }
    current_block = cur_block;
    current_stage = cur_stage;
    block_times[cur_block]->setText(QString("%1:%2").arg(block_time / 60, 2, 10, QChar('0')).arg(block_time % 60, 2, 10, QChar('0')));
}
