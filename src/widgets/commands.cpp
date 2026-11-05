#include "commands.h"
#include <QLayout>
#include "AircraftManager.h"
#include "pprz_dispatcher.h"
#include "aircraft.h"
#include <QSettings>
#include "gcs_utils.h"

Commands::Commands(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id)
{
    setObjectName("commands");
    setAttribute(Qt::WA_TranslucentBackground);     // rounded corners (the popup is frameless)
    auto lay = new QVBoxLayout(this);
    lay->setSizeConstraint(QLayout::SetFixedSize);
    lay->setContentsMargins(12, 14, 12, 12);
    lay->setSpacing(6);

    auto name = AircraftManager::get()->getAircraft(ac_id)->name();
    auto name_label = new QLabel(name, this);
    name_label->setStyleSheet("font-weight: bold; font-size: 14px;");
    lay->addWidget(name_label);

    auto spe_cmd_lay = new QGridLayout();
    addSpecialCommands(spe_cmd_lay);
    lay->addLayout(spe_cmd_lay);

    auto fp_caption = new QLabel("Flight plan", this);
    fp_caption->setObjectName("commands_caption");
    lay->addWidget(fp_caption);

    auto fp_lay = new QGridLayout();
    addFlightPlanButtons(fp_lay);
    lay->addLayout(fp_lay);
    fp_caption->setVisible(fp_lay->count() > 0);

    auto set_caption = new QLabel("Settings", this);
    set_caption->setObjectName("commands_caption");
    lay->addWidget(set_caption);

    auto set_lay = new QGridLayout();
    addSettingsButtons(set_lay);
    lay->addLayout(set_lay);
    set_caption->setVisible(set_lay->count() > 0);

    target_alt = static_cast<float>(AircraftManager::get()->getAircraft(ac_id)->getFlightPlan()->getDefaultAltitude());
    connect(PprzDispatcher::get(), &PprzDispatcher::nav_status, this, &Commands::updateTargetAlt);
}

void Commands::paintEvent(QPaintEvent* e) {
    // a card with a bar in the aircraft color at the top
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QColor background = palette().color(QPalette::Window);
    QColor border = palette().color(QPalette::Mid);
    p.setPen(QPen(border, 1));
    p.setBrush(background);
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(AircraftManager::get()->getAircraft(ac_id)->getColor());
    p.drawRoundedRect(QRect(0, 0, width(), 6), 3, 3);

    QWidget::paintEvent(e);
}

void Commands::addFlightPlanButtons(QGridLayout* fp_buttons_layout) {
    auto groups = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan()->getGroups();
    int col = 0;
    for(auto &group: groups) {
        int row = 0;
        for(auto block: group->blocks) {
            QString icon = block->getIcon();
            QString txt = block->getText();
            QPushButton* b = nullptr;

            QString icon_path = appConfig()->value("GCS_ICONS_PATH").toString() + "/" + icon;

            if(icon != "" && QFile::exists(icon_path)) {
                b = new QPushButton(this);
                b->setIcon(QIcon(icon_path));
                if(txt != "") {
                    b->setToolTip(txt);
                }
            } else if (txt != "") {
                b = new QPushButton(txt, this);
            }

            if(b != nullptr) {
                fp_buttons_layout->addWidget(b, row, col);
                  connect(b, &QPushButton::clicked, this,
                    [=,this]() {
                        pprzlink::Message msg(PprzDispatcher::get()->getDict()->getDefinition("JUMP_TO_BLOCK"));
                        msg.addField("ac_id", ac_id);
                        msg.addField("block_id", block->getNo());
                        PprzDispatcher::get()->sendMessage(msg);
                });
            }
            ++row;
        }
        ++col;
    }
}

void Commands::addSettingsButtons(QGridLayout* settings_buttons_layout) {
    auto groups = AircraftManager::get()->getAircraft(ac_id)->getSettingMenu()->getButtonGroups();

    int col = 0;
    for(auto &group: groups) {
        int row = 0;
        for(auto &sb: group->buttons) {
            QString icon = sb->icon;
            QString name = sb->name;
            QPushButton* b = nullptr;

            QString icon_path = appConfig()->value("GCS_ICONS_PATH").toString() + "/" + icon;

            if(icon != "" && QFile::exists(icon_path)) {
                b = new QPushButton(this);
                b->setIcon(QIcon(icon_path));
                if(name != "") {
                    b->setToolTip(name);
                }
            } else if (name != "") {
                b = new QPushButton(name, this);
            }

            if(b != nullptr) {
                settings_buttons_layout->addWidget(b, row, col);
                  connect(b, &QPushButton::clicked, this,
                    [=,this]() {
                        AircraftManager::get()->getAircraft(ac_id)->setSetting(sb->setting_no, sb->value);
                });
            }
            ++row;
        }
        ++col;
    }
}

void Commands::addSpecialCommands(QGridLayout* glay) {
    //special_commands_layout->setSizeConstraint(QLayout::SetFixedSize);
    glay->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 0);
    glay->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 4);

    auto ac = AircraftManager::get()->getAircraft(ac_id);
    auto settings = ac->getSettingMenu()->getAllSettings();

    for(auto set:settings) {
        if(set->getName() == "autopilot.launch") {
            addCommandButton(glay, "launch.png", 0, 1, [=,this]() mutable {
                ac->setSetting(set, 1);
            }, "Launch");
        }

        if(set->getName() == "autopilot.kill_throttle") {
            addCommandButton(glay, "kill.png", 0, 2, [=,this]() mutable {
                ac->setSetting(set, 1);
            }, "Kill throttle");

            addCommandButton(glay, "resurrect.png", 0, 3, [=,this]() mutable {
                ac->setSetting(set, 0);
            }, "Resurrect throttle");
        }

        if(set->getName() == "altitude" || set->getName() == "flight_altitude") {
          float altShiftPlus = ac->getAirframe()->getAltShiftPlus();
          float altShiftMinus = ac->getAirframe()->getAltShiftMinus();
          float altShiftPlusPlus = ac->getAirframe()->getAltShiftPlusPlus();

          addCommandButton(glay, "up.png", 1, 1, [=,this]() mutable {
                ac->setSetting(set, target_alt + altShiftPlus);
            }, QString("+%1").arg(altShiftPlus, 0, 'f', 1));

            addCommandButton(glay, "down.png", 1, 2, [=,this]() mutable {
                ac->setSetting(set, target_alt + altShiftMinus);
            }, QString("%1").arg(altShiftMinus, 0, 'f', 1));

            addCommandButton(glay, "upup.png", 1, 3, [=,this]() mutable {
                ac->setSetting(set, target_alt + altShiftPlusPlus);
            }, QString("+%1").arg(altShiftPlusPlus, 0, 'f', 1));
        }

        if(set->getName() == "inc. shift") {
            addCommandButton(glay, "left.png", 2, 1, [=,this]() mutable {
                ac->setSetting(set, -5);
            }, "-5");

            addCommandButton(glay, "recenter.png", 2, 2, [=,this]() mutable {
                ac->setSetting(set, 0);
            }, "0");

            addCommandButton(glay, "right.png", 2, 3, [=,this]() mutable {
                qDebug() << "current value: " << set->getUserValue() << "  (+5)";
                ac->setSetting(set, 5);
            }, "+5");
        }
    }

}

void Commands::addCommandButton(QGridLayout* glay,QString icon, int row, int col, std::function<void()> callback, QString tooltip) {
    auto button = new QToolButton(this);
    QString icon_path = appConfig()->value("GCS_ICONS_PATH").toString() + "/" + icon;
    button->setIcon(QIcon(icon_path));
    button->setToolTip(tooltip);
    glay->addWidget(button, row, col);
    connect(button, &QToolButton::clicked, callback);
}

void Commands::updateTargetAlt(pprzlink::Message msg) {
    QString id;
    msg.getField("ac_id", id);

    if(ac_id == id) {
        msg.getField("target_alt", target_alt);
    }
}

