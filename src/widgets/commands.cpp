#include "commands.h"
#include <QLayout>
#include "AircraftManager.h"
#include "pprz_dispatcher.h"
#include "aircraft.h"

Commands::Commands(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id)
{
    auto lay = new QVBoxLayout(this);
    lay->setSizeConstraint(QLayout::SetFixedSize);

    auto name = AircraftManager::get()->getAircraft(ac_id).name();
    auto name_label = new QLabel(name, this);
    name_label->setStyleSheet("font-weight: bold");
    lay->addWidget(name_label);

    auto spe_cmd_lay = new QGridLayout();
    addSpecialCommands(spe_cmd_lay);
    lay->addLayout(spe_cmd_lay);

    auto line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    lay->addWidget(line);

    auto fp_lay = new QGridLayout();
    addFlightPlanButtons(fp_lay);
    lay->addLayout(fp_lay);

    auto line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    lay->addWidget(line2);

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
                        AircraftManager::get()->getAircraft(ac_id).setSetting(sb->setting_no, sb->value);
                });
            }
            ++row;
        }
        --col;
        row = 0;
    }
}

void Commands::addSpecialCommands(QGridLayout* glay) {
    //special_commands_layout->setSizeConstraint(QLayout::SetFixedSize);
    glay->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 0);
    glay->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 4);

    auto ac = AircraftManager::get()->getAircraft(ac_id);
    auto settings = ac.getSettingMenu()->getAllSettings();

    for(auto set:settings) {
        if(set->getName() == "autopilot.launch") {
            addCommandButton(glay, "launch.png", 0, 1, [=]() mutable {
                ac.setSetting(set, 1);
            });
        }

        if(set->getName() == "autopilot.kill_throttle") {
            addCommandButton(glay, "kill.png", 0, 2, [=]() mutable {
                ac.setSetting(set, 1);
            });

            addCommandButton(glay, "resurrect.png", 0, 3, [=]() mutable {
                ac.setSetting(set, 0);
            });
        }

        if(set->getName() == "altitude") {
            addCommandButton(glay, "up.png", 1, 1, [=]() mutable {
                ac.setSetting(set, set->getValue() + ac.getAirframe().getAltShiftPlus());
            });

            addCommandButton(glay, "down.png", 1, 2, [=]() mutable {
                ac.setSetting(set, set->getValue() + ac.getAirframe().getAltShiftMinus());
            });

            addCommandButton(glay, "upup.png", 1, 3, [=]() mutable {
                ac.setSetting(set, set->getValue() + ac.getAirframe().getAltShiftPlusPlus());
            });
        }

        if(set->getName() == "inc. shift") {
            addCommandButton(glay, "left.png", 2, 1, [=]() mutable {
                qDebug() << "current value: " << set->getValue() << "  (-5)";
                ac.setSetting(set, -5);
            });

            addCommandButton(glay, "recenter.png", 2, 2, [=]() mutable {
                qDebug() << "current value: " << set->getValue() << "  recenter";
                ac.setSetting(set, 0);
            });

            addCommandButton(glay, "right.png", 2, 3, [=]() mutable {
                qDebug() << "current value: " << set->getValue() << "  (+5)";
                ac.setSetting(set, 5);
            });
        }
    }

}

void Commands::addCommandButton(QGridLayout* glay,QString icon, int row, int col, std::function<void()> callback) {
    auto button = new QToolButton(this);
    QString icon_path = qApp->property("PATH_GCS_ICON").toString() + "/" + icon;
    button->setIcon(QIcon(icon_path));
    glay->addWidget(button, row, col);
    connect(button, &QToolButton::clicked, callback);
}
