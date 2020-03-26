#include "mapstrip.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QDebug>
#include <string>
#include <AircraftManager.h>
#include <QFontDatabase>
#include <QApplication>
#include <QDebug>
#include "dispatcher_ui.h"

MapStrip::MapStrip(QString ac_id, QWidget *parent) :
    QWidget(parent), ac_id(ac_id), lock(false)
{
    QFont fixedFont;
    fixedFont.setFamily(QString::fromUtf8("Ubuntu Mono"));

    layout = new QGridLayout(this);
    fp_buttons_layout = new QGridLayout();

    layout->addLayout(fp_buttons_layout, 1, 3);
    setLayoutDirection(Qt::LayoutDirection::RightToLeft);

    ac_name = new QLabel(AircraftManager::get()->getAircraft(ac_id).name(), this);
    ac_name->setFont(fixedFont);
    layout->addWidget(ac_name, 0, 0);

    bat_label = new QLabel("BAT : UNK", this);
    bat_label->setFont(fixedFont);
    layout->addWidget(bat_label, 1, 0);

    link_label = new QLabel("LINK: UNK", this);
    link_label->setFont(fixedFont);
    layout->addWidget(link_label, 2, 0);

    speed_label =  new QLabel("SPEED: UNK", this);
    speed_label->setFont(fixedFont);
    layout->addWidget(speed_label, 1, 1);

    height_label = new QLabel("AGL  : UNK", this);
    height_label->setFont(fixedFont);
    layout->addWidget(height_label, 2, 1);

    ap_mode = new QLabel("MODE : UNK", this);
    ap_mode->setFont(fixedFont);
    layout->addWidget(ap_mode, 1, 2);

    cur_block = new QLabel("MODE : UNK", this);
    cur_block->setFont(fixedFont);
    layout->addWidget(cur_block, 2, 2);

    auto groups = AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getGroups();
    int col = static_cast<int>(groups.size());
    int row = 0;
    for(auto group: groups) {
        for(auto block: group->blocks) {
            QString icon = block->getIcon().c_str();
            QString txt = block->getText().c_str();
            QPushButton* b = nullptr;

            if(icon != "") {
                b = new QPushButton(this);
                QString icon_path = qApp->property("PATH_GCS_ICON").toString() + "/" + icon;
                b->setIcon(QIcon(icon_path));
                if(txt != "") {
                    b->setToolTip(txt);
                }
            } else if (txt != "") {
                b = new QPushButton(txt, this);
                b->setToolTip(txt);
            }

            if(b != nullptr) {
                fp_buttons_layout->addWidget(b, row, col);
                  connect(b, &QPushButton::clicked,
                    [=]() {
                        pprzlink::Message msg(PprzDispatcher::get()->getDict()->getDefinition("JUMP_TO_BLOCK"));
                        msg.addField("ac_id", ac_id.toStdString());
                        msg.addField("block_id", block->getNo());
                        PprzDispatcher::get()->sendMessage(msg);
                });
            }
            ++row;
        }
        --col;
        row = 0;
    }

    connect(PprzDispatcher::get(), &PprzDispatcher::flight_param, this, &MapStrip::updateFlightParams);
    connect(PprzDispatcher::get(), &PprzDispatcher::ap_status, this, &MapStrip::updateApStatus);
    connect(PprzDispatcher::get(), &PprzDispatcher::nav_status, this, &MapStrip::updateNavStatus);



    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

MapStrip::~MapStrip()
{

}


void MapStrip::paintEvent(QPaintEvent *event) {

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.setFillRule( Qt::WindingFill );

    //int radius = 20;
    QRect rect = event->rect();
//    QRect br = QRect(rect.width()-radius, rect.height()-radius, radius, radius);
//    QRect tr = QRect(rect.width()-radius, 0, radius, radius);

    path.addRect(rect);
    //path.addRoundedRect(rect, radius, radius);
//    path.addRect(br);
//    path.addRect(tr);

    p.setPen(Qt::NoPen);

    QColor color = AircraftManager::get()->getAircraft(ac_id).getColor();
    int hue = color.hue();
    int sat = color.saturation();
    color.setHsv(hue, static_cast<int>(sat*0.75), 255);
    color.setAlpha(200);

    p.fillPath(path, color);
    p.drawPath(path);

    QWidget::paintEvent(event);
}


void MapStrip::mousePressEvent(QMouseEvent *event) {
    qDebug() << "press!";
    lock = !lock;
    QWidget::mousePressEvent(event);
}


void MapStrip::enterEvent(QEvent *event) {
    emit(DispatcherUi::get()->ac_selected(ac_id));
    ap_mode->show();
    for (int i = 0; i < fp_buttons_layout->count(); ++i)
    {
      QWidget *widget = fp_buttons_layout->itemAt(i)->widget();
      if (widget != nullptr)
      {
        widget->setVisible(true);
      }
    }
    QWidget::enterEvent(event);
}

void MapStrip::leaveEvent(QEvent *event) {
    if(!lock) {
        ap_mode->hide();
        for (int i = 0; i < fp_buttons_layout->count(); ++i)
        {
          QWidget *widget = fp_buttons_layout->itemAt(i)->widget();
          if (widget != nullptr)
          {
            widget->setVisible(false);
          }
        }
    }
    QWidget::leaveEvent(event);
}

void MapStrip::updateFlightParams(pprzlink::Message msg) {
    std::string id;
    msg.getField("ac_id", id);
    if(ac_id != id.c_str()) {
        return;
    }

    float speed, agl, airspeed;
    msg.getField("speed", speed);
    msg.getField("agl", agl);
    msg.getField("airspeed", airspeed);

    speed_label->setText("SPEED: " + QString::number(speed, 'f', 1) + "m/s");
    height_label->setText("AGL  : " + QString::number(static_cast<int>(agl)) + "m");
}


void MapStrip::updateApStatus(pprzlink::Message msg) {
    std::string id;
    msg.getField("ac_id", id);
    if(ac_id != id.c_str()) {
        return;
    }
    std::string mode;
    msg.getField("ap_mode", mode);

    ap_mode->setText("MODE : " + QString(mode.c_str()));
}

void MapStrip::updateNavStatus(pprzlink::Message msg) {
    std::string id;
    msg.getField("ac_id", id);
    if(ac_id != id.c_str()) {
        return;
    }
    uint8_t block_no;
    msg.getField("cur_block", block_no);

    auto block_name = AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getBlock(block_no)->getName();
    (void)block_name;
    cur_block->setText("BLOCK : " + QString(block_name.c_str()));
}
