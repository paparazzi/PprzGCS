#include "flightplan_viewer.h"
#include "dispatcher_ui.h"
#include "AircraftManager.h"
#include "mbpushbutton.h"
#include <QDebug>
#include "pprz_dispatcher.h"

FlightPlanViewer::FlightPlanViewer(QWidget *parent) : QWidget(parent)
{
    layout = new QVBoxLayout(this);
    head_row = new QHBoxLayout();
    stack_layout = new QStackedLayout();

    layout->addLayout(head_row);

    QFrame *line;
    line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    layout->addLayout(stack_layout);


    ac_combo = new QComboBox(this);
    head_row->addWidget(ac_combo);

    auto header_bt = new QPushButton("Headers");
    auto wp_bt = new QPushButton("Waypoints");
    auto exceptions_bt = new QPushButton("Exceptions");
    head_row->addWidget(header_bt);
    head_row->addWidget(wp_bt);
    head_row->addWidget(exceptions_bt);



    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &FlightPlanViewer::handleNewAC);
    connect(ac_combo, qOverload<int>(&QComboBox::currentIndexChanged),
        [=](int ix) {
            this->repaint();
            DispatcherUi::get()->ac_selected(ac_ids[ix]);
            stack_layout->setCurrentIndex(ix);
        });

    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this,
        [=](QString ac_id) {
        for(auto ids: ac_ids) {
            if(ids.second == ac_id) {
                ac_combo->setCurrentIndex(ids.first);
                stack_layout->setCurrentIndex(ids.first);
            }
        }
    });

}

void FlightPlanViewer::handleNewAC(QString ac_id) {
    ac_ids[ac_combo->count()] = ac_id;
    auto ac = AircraftManager::get()->getAircraft(ac_id);

    FPInnerViewer* innerView = new FPInnerViewer(ac, this);
    stack_layout->addWidget(innerView);

    ac_combo->addItem(ac.name());

    setStyleSheet("QWidget {frame: 2px}");

}

void FlightPlanViewer::paintEvent(QPaintEvent *event) {
    if(ac_combo->count() > 0 && ac_ids[ac_combo->currentIndex()] != "") {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRect(event->rect());
        p.setPen(Qt::NoPen);

        QColor color = AircraftManager::get()->getAircraft(ac_ids[ac_combo->currentIndex()]).getColor();
        int hue = color.hue();
        int sat = color.saturation();
        color.setHsv(hue, static_cast<int>(sat*0.5), 255);

        p.fillPath(path, color);
        p.drawPath(path);
    }

    QWidget::paintEvent(event);
}


FPInnerViewer::FPInnerViewer(Aircraft& acr, QWidget *parent) : QWidget(parent),
    ac(acr), block_locked(false)
{
    layout = new QVBoxLayout(this);
    blocks_layout = new FlowLayout();


    layout->addLayout(blocks_layout);
    stages_list = new QListWidget(this);
    layout->addWidget(stages_list);

    for(auto block: ac.getFlightPlan().getBlocks()) {
        QString icon = block->getIcon().c_str();
        QString txt = block->getText().c_str();
        MbPushButton* b = new MbPushButton(block->getName().c_str(), this);
        if(icon != "") {
            QString icon_path = qApp->property("PATH_GCS_ICON").toString() + "/" + icon;
            b->setIcon(QIcon(icon_path));
        }
        if(txt != "") {
            b->setToolTip(txt);
        }
        blocks_layout->addWidget(b);

        auto ac_id = ac.getId().toStdString();
        connect(b, &MbPushButton::clicked,
            [=]() {
                pprzlink::Message msg(PprzDispatcher::get()->getDict()->getDefinition("JUMP_TO_BLOCK"));
                msg.addField("ac_id", ac_id);
                msg.addField("block_id", block->getNo());
                PprzDispatcher::get()->sendMessage(msg);
            });

        connect(b, &MbPushButton::rightClicked,
            [=]() {
                auto old_block = bLocked;

                if(!block_locked) {
                    block_locked = true;
                    bLocked = block;
                } else {
                    if(bLocked == block) {
                        // click a again on the locked block unlock it
                        block_locked = false;
                    } else {
                        // locked the clicked block
                        bLocked = block;
                    }
                }

                if(old_block != bLocked) {
                    update_stage_display();
                }
            });

        connect(b, &MbPushButton::enter,
            [=]() {
                if(!block_locked) {
                    bLocked = block;
                    update_stage_display();
                }
            });

    }

}

void FPInnerViewer::update_stage_display() {
    stages_list->clear();
    for(auto stage: bLocked->getStages()) {
        string txt = stage.instruction.c_str();
        if(stage.attributes.size() > 0) {
            txt += ": ";
            for(auto att: stage.attributes) {
                txt += att.first + "=\"" + att.second + "\"  ";
            }
        }

        stages_list->addItem(txt.c_str());
        qDebug() << stage.instruction.c_str();
    }
}

void FPInnerViewer::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QSize si = blocks_layout->sizeHint();
    QPainter p(this);
    QRect rect = blocks_layout->geometry();
    qDebug() << rect << "  " << si;

    p.setPen(QPen(Qt::green));
    p.drawRect(rect);

    p.setPen(QPen(Qt::blue));
    p.drawRect(QRect(rect.topLeft(), si));



}
