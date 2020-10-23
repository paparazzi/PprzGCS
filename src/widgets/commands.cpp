#include "commands.h"
#include <QLayout>
#include "AircraftManager.h"
#include "pprz_dispatcher.h"

Commands::Commands(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id)
{
    auto lay = new QVBoxLayout(this);
    lay->setSizeConstraint(QLayout::SetFixedSize);

    auto name = AircraftManager::get()->getAircraft(ac_id).name();
    auto name_label = new QLabel(name, this);
    name_label->setStyleSheet("font-weight: bold");
    lay->addWidget(name_label);

    auto line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    lay->addWidget(line);

    auto fp_lay = new QGridLayout();
    addFlightPlanButtons(fp_lay);
    lay->addLayout(fp_lay);

    auto set_lay = new QGridLayout();
    addSettingsButtons(set_lay);
    lay->addLayout(set_lay);

}

void Commands::paintEvent(QPaintEvent* e) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRect(rect());
    p.setPen(Qt::NoPen);

    QColor color = AircraftManager::get()->getAircraft(ac_id).getColor();
    int hue = color.hue();
    int sat = color.saturation();
    color.setHsv(hue, static_cast<int>(sat*0.2), 255);

    p.fillPath(path, color);
    p.drawPath(path);

    QWidget::paintEvent(e);
}


void Commands::addFlightPlanButtons(QGridLayout* fp_buttons_layout) {
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
}

void Commands::addSettingsButtons(QGridLayout* settings_buttons_layout) {
    vector<shared_ptr<SettingMenu::ButtonGroup>> groups = AircraftManager::get()->getAircraft(ac_id).getSettingMenu()->getButtonGroups();

    int col = static_cast<int>(groups.size());
    int row = 0;
    for(auto group: groups) {
        for(auto sb: group->buttons) {
            QString icon = sb->icon.c_str();
            QString name = sb->name.c_str();
            QPushButton* b = nullptr;

            if(icon != "") {
                b = new QPushButton(this);
                QString icon_path = qApp->property("PATH_GCS_ICON").toString() + "/" + icon;
                b->setIcon(QIcon(icon_path));
                if(name != "") {
                    b->setToolTip(name);
                }
            } else if (name != "") {
                b = new QPushButton(name, this);
                b->setToolTip(name);
            }

            if(b != nullptr) {
                settings_buttons_layout->addWidget(b, row, col);
                  connect(b, &QPushButton::clicked,
                    [=]() {
                        pprzlink::Message msg(PprzDispatcher::get()->getDict()->getDefinition("DL_SETTING"));
                        msg.addField("ac_id", ac_id.toStdString());
                        msg.addField("index", sb->setting_no);
                        msg.addField("value", sb->value);
                        PprzDispatcher::get()->sendMessage(msg);
                });
            }
            ++row;
        }
        --col;
        row = 0;
    }
}
