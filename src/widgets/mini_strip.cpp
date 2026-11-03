#include "card_dialog.h"
#include "mini_strip.h"
#include "AircraftManager.h"
#include "gcs_utils.h"
#include "dispatcher_ui.h"
#include "mini_strip.h"
#include <QDebug>
#include <QSettings>
#include <QApplication>

// Lost links and battery level are shown with the properties "link" (on the strip) and "bat"
// (on the voltage), styled by the application style sheet (light and night themes). These
// colors are used when the style sheet does not style them (an older custom style).
static const QString STRIP_STYLE =
    "QPushButton#strip_kill {border: 0; background: transparent; padding: 0;}"
    "QPushButton#strip_kill:hover {background: rgba(128, 128, 128, 60); border-radius: 12px;}"
    // the row of the status icons: the strip color behind it (selected: blue)
    "#mini_strip QWidget#status_row {background: transparent;}"
    "QLabel#value {font-family: \"Monospace\"; font-size: 14px;}"
    "QLabel#caption {font-size: 11px;}"
    "QLabel#name {font-weight: bold; font-size: 15px;}"
    "QPushButton#cell {border: 0; background: transparent; padding: 0; text-align: left;}"
    "QPushButton#cell:hover {background: rgba(128, 128, 128, 40);}";
static const QString STRIP_FALLBACK =
    "#mini_strip[selected=\"true\"] {border: 2px solid #1d5fc9; background: #f5f8fd;}"
    "#mini_strip[link=\"warning\"] {border: 3px solid #c27c0e; background: #fff4e0;}"
    "#mini_strip[link=\"lost\"] {border: 3px solid #b42318; background: #fde8e6;}"
    "QLabel[bat=\"low\"] {color: #8a4b00; font-weight: bold;}"
    "QLabel[bat=\"critic\"] {color: #b42318; font-weight: bold;}"
    "QLabel#caption {color: #30363d;}"
    "QLabel#status_text {color: #b42318; font-weight: bold;}";

static void restyle(QWidget* w) {
    w->style()->unpolish(w);
    w->style()->polish(w);
}

const QMap<QString, QString> MiniStrip::mode_colors{
    // fixedwings
    {"HOME", "#ff0000"}, {"NOGPS", "#ff0000"},
    {"MANUAL", "#ffa500"},
    {"AUTO1", "#10f0e0"},
    {"AUTO2", "#7ef17e"},

    // rotorcrafts
    {"KILL", "#ff0000"}, {"SAFE", "#ff0000"}, /*HOME, */

    {"RATE", "#ffa500"}, {"ATT", "#ffa500"},
    {"R_RCC", "#ffa500"},{"A_RCC", "#ffa500"},{"ATT_C", "#ffa500"},{"R_ZH", "#ffa500"},

    {"HOVER", "#10f0e0"}, {"HOV_C", "#10f0e0"}, {"H_ZH", "#10f0e0"},
    {"RC_D", "#10f0e0"}, {"CF", "#10f0e0"}, {"FWD", "#10f0e0"},
    {"MODULE", "#10f0e0"}, {"FLIP", "#10f0e0"}, {"A_ZH", "#10f0e0"},

    {"NAV", "#7ef17e"}, {"GUIDED", "#70c070"},
};

MiniStrip::MiniStrip(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id), icons_size(QSize(30, 30)), time_mode(TimeMode::FLIGHT_TIME),
    alt_mode(true), speed_mode(SpeedMode::GROUND_SPEED)
{
    // Two rows: name, mode, block and time; then labeled values. The status icons (link, GPS,
    // RC, IMU) only show when something is wrong. Clicking time, altitude or speed switches
    // what they show; the caption says which.
    setContentsMargins(0, 0, 0, 0);
    setMinimumWidth(360);
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    auto vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(8, 6, 8, 6);
    vbox->setSpacing(4);

    /////////// first row: name, mode, block, time ///////////
    auto row1 = new QHBoxLayout();
    row1->setSpacing(8);
    auto ac_name = new QLabel(ac->name(), this);
    ac_name->setObjectName("name");
    row1->addWidget(ac_name);

    ap_mode_button = new QPushButton("", this);
    ap_mode_button->setToolTip("AP mode");
    row1->addWidget(ap_mode_button);
    auto settings = ac->getSettingMenu()->getAllSettings();
    for(auto setting: settings) {
        if(setting->getName() == "autopilot.mode") {
            connect(ap_mode_button, &QPushButton::clicked, this, [=,this](){
                AircraftManager::get()->getAircraft(ac_id)->setSetting(setting, "AUTO2");
            });
            break;
        }
    }

    block_icon = new QLabel(this);
    block_label = new QLabel(this);
    block_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    row1->addWidget(block_icon);
    row1->addWidget(block_label, 1);

    // kill the throttle (or resurrect it), after a confirmation: when the aircraft has the setting
    for(auto setting: settings) {
        if(setting->getName() == "autopilot.kill_throttle") {
            // a small round power button: red (kill), green while killed (resurrect)
            kill_button = new QPushButton(this);
            kill_button->setIcon(QIcon(user_or_app_path("pictures/strip_kill.svg")));
            kill_button->setIconSize(QSize(20, 20));
            kill_button->setFixedSize(24, 24);
            kill_button->setObjectName("strip_kill");
            kill_button->setCursor(Qt::PointingHandCursor);
            kill_button->setToolTip("Kill the throttle (asks first)");
            // placed on the values row, after the throttle (below)
            connect(kill_button, &QPushButton::clicked, this, [=,this]() {
                bool killed = kill_button->property("killed").toBool();
                QString name = AircraftManager::get()->getAircraft(ac_id)->name();
                bool ok = killed ?
                    CardDialog::ask(this, "Resurrect the throttle of " + name + "?",
                                    "The motors can start again.", "Resurrect", CardDialog::PRIMARY) :
                    CardDialog::ask(this, "Kill the throttle of " + name + "?",
                                    "The motors stop at once, also in flight.", "Kill", CardDialog::DANGER);
                if(ok && AircraftManager::get()->aircraftExists(ac_id)) {
                    AircraftManager::get()->getAircraft(ac_id)->setSetting(setting, killed ? 0.f : 1.f);
                }
            });
            break;
        }
    }

    QLabel* time_caption;
    time_button = makeCell(time_caption, flight_time_label, false);
    time_caption->setText("Flight");
    flight_time_label->setText("00:00:00");
    time_button->setToolTip("Flight time (click: block, stage time)");
    row1->addWidget(time_button);
    connect(time_button, &QPushButton::clicked, this,
        [=,this]() {
        switch (time_mode) {
        case FLIGHT_TIME:
            time_mode = BLOCK_TIME;
            time_caption->setText("Block");
            break;
        case BLOCK_TIME:
            time_mode = STAGE_TIME;
            time_caption->setText("Stage");
            break;
        case STAGE_TIME:
            time_mode = FLIGHT_TIME;
            time_caption->setText("Flight");
            break;
        }
        updateData();
        });
    vbox->addLayout(row1);

    /////////// second row: values ///////////
    auto row2 = new QGridLayout();
    row2->setHorizontalSpacing(10);
    row2->setVerticalSpacing(0);

    QLabel* bat_caption;
    auto bat_cell = makeCell(bat_caption, bat_label, true);
    bat_caption->setText("Battery");
    bat_cell->setFocusPolicy(Qt::NoFocus);
    auto bnbc = ac->getAirframe()->getDefine("BAT_NB_CELLS");
    bat_nb_cells = bnbc.has_value() ? bnbc->value.toInt() : 0;
    if(bat_nb_cells != 0) {
        // per cell or the total voltage (click)
        bat_cell->setToolTip("Battery voltage per cell (click: total)");
        connect(bat_cell, &QPushButton::clicked, this, [=,this]() {
            bat_per_cell = !bat_per_cell;
            bat_cell->setToolTip(bat_per_cell ? "Battery voltage per cell (click: total)" :
                                                "Battery voltage, total (click: per cell)");
            updateData();
        });
    } else {
        bat_cell->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
    row2->addWidget(bat_cell, 0, 0);

    QLabel* alt_caption;
    alt_button = makeCell(alt_caption, alt_label, true);
    alt_caption->setText("AGL | to target");
    alt_button->setToolTip("Height above ground, and to the target altitude (click: MSL)");
    row2->addWidget(alt_button, 0, 1);
    connect(alt_button, &QPushButton::clicked, this,
        [=,this]() {
            alt_mode = !alt_mode;
            alt_caption->setText(alt_mode ? "AGL | to target" : "MSL | to target");
            alt_button->setToolTip(alt_mode ?
                "Height above ground, and to the target altitude (click: MSL)" :
                "Altitude MSL, and to the target altitude (click: AGL)");
            updateData();
        });

    QLabel* speed_caption;
    speed_button = makeCell(speed_caption, speed_label, true);
    speed_caption->setText("Ground speed");
    speed_button->setToolTip("Click: ground speed, airspeed, climb");
    row2->addWidget(speed_button, 0, 2);
    connect(speed_button, &QPushButton::clicked, this,
        [=,this]() {
            switch (speed_mode) {
            case GROUND_SPEED:
                speed_mode = AIR_SPEED;
                speed_caption->setText("Airspeed");
                break;
            case AIR_SPEED:
                speed_mode = VERTICAL_SPEED;
                speed_caption->setText("Climb");
                break;
            case VERTICAL_SPEED:
                speed_mode = GROUND_SPEED;
                speed_caption->setText("Ground speed");
                break;
            }
            updateData();
        });

    auto th_cell = makeCell(throttle_caption, throttle_label, true);
    throttle_caption->setText("Throttle");
    th_cell->setAttribute(Qt::WA_TransparentForMouseEvents);
    // the values next to each other with the same small gap; the spare width after them, then
    // the throttle aligned right, close to the kill button on the right edge
    row2->setColumnStretch(3, 1);
    row2->addWidget(th_cell, 0, 4);
    for(auto l: {throttle_caption, throttle_label}) {
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
    if(kill_button != nullptr) {
        row2->addWidget(kill_button, 0, 5, Qt::AlignRight | Qt::AlignVCenter);
    }
    compact_icons = new QHBoxLayout();
    compact_icons->setSpacing(2);
    row1->insertLayout(row1->indexOf(time_button), compact_icons);   // before the time
    vbox->addLayout(row2);
    captions = {time_caption, bat_caption, alt_caption, speed_caption, throttle_caption};
    value_cells = {bat_cell, alt_button, speed_button, th_cell};
    for(auto cell: QList<QPushButton*>{bat_cell, alt_button, speed_button, th_cell, time_button}) {
        cell->installEventFilter(this);
    }

    ///// Link, GPS, RC, IMU: only when not ok //////
    auto status_lay = new QHBoxLayout();
    status_lay->setSpacing(6);
    QSize status_size(22, 22);
    link_ok = QIcon(":/pictures/link_ok.svg");
    link_warning = QIcon(":/pictures/link_warning.svg");
    link_lost = QIcon(":/pictures/link_nok.svg");
    gps_lost = QIcon(":/pictures/gps_nok.svg");
    gps_ok = QIcon(":/pictures/gps_ok.svg");
    gps_dgps = QIcon(":/pictures/gps_dgps.svg");
    gps_rtk = QIcon(":/pictures/gps_rtk.svg");
    rc_ok = QIcon(":/pictures/rc_ok.svg");
    rc_lost = QIcon(":/pictures/rc_nok.svg");
    imu_ok = QIcon(":/pictures/imu_ok.svg");
    imu_lost = QIcon(":/pictures/imu_nok.svg");
    link_icon = new QLabel(this);
    gps_icon = new QLabel(this);
    rc_icon = new QLabel(this);
    imu_icon = new QLabel(this);
    status_text = new QLabel(this);
    status_text->setObjectName("status_text");
    for(auto l: {link_icon, gps_icon, rc_icon, imu_icon}) {
        l->setFixedSize(status_size);
        l->setScaledContents(true);
        status_lay->addWidget(l);
    }
    status_lay->addWidget(status_text, 1);
    status_layout = status_lay;
    status_row = new QWidget(this);
    status_row->setObjectName("status_row");
    status_row->setLayout(status_lay);
    status_lay->setContentsMargins(0, 0, 0, 0);
    vbox->addWidget(status_row);

    setObjectName("mini_strip");
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet(STRIP_STYLE + (qApp->styleSheet().contains("mini_strip") ? "" : STRIP_FALLBACK));
    // the time keeps its width (measured with the style sheet fonts)
    for(auto l: time_button->findChildren<QLabel*>()) {
        l->ensurePolished();
    }
    time_button->setMinimumWidth(time_button->layout()->sizeHint().width() + 8);
    time_button->setProperty("full_width", time_button->minimumWidth());

    // the selected aircraft's strip is highlighted; clicking a strip selects its aircraft
    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this, [=,this](QString id) {
        bool selected = id == this->ac_id;
        if(property("selected").toBool() != selected) {
            setProperty("selected", selected);
            restyle(this);
        }
    });
    if(DispatcherUi::get()->getSelectedAcId() == ac_id) {
        setProperty("selected", true);
    }

    // until the first data
    for(auto l: {bat_label, alt_label, speed_label, throttle_label}) {
        l->setText(QString::fromUtf8("\xE2\x80\x94"));
    }
    updateAp("");
    all_icons = getAppSettings().value("strips/status_icons", false).toBool();
    updateStatusRow();

    auto ac_status = AircraftManager::get()->getAircraft(ac_id)->getStatus();

    connect(ac_status, &AircraftStatus::engine_status, this, &MiniStrip::updateData);
    connect(ac_status, &AircraftStatus::flight_param, this, &MiniStrip::updateData);
    connect(ac_status, &AircraftStatus::telemetry_status, this, &MiniStrip::updateData);
    connect(ac_status, &AircraftStatus::fly_by_wire, this, &MiniStrip::updateData);
    connect(ac_status, &AircraftStatus::ap_status, this, &MiniStrip::updateData);
    connect(ac_status, &AircraftStatus::nav_status, this, &MiniStrip::updateData);
    connect(ac_status->getWatcher(), &AircraftWatcher::bat_status, this, &MiniStrip::handle_bat_status);
}

/**
 * A value with its caption: a flat button (clickable to change what it shows), the caption
 * above (vertical) or before the value.
 */
QPushButton* MiniStrip::makeCell(QLabel*& caption, QLabel*& value, bool vertical) {
    auto cell = new QPushButton(this);
    cell->setObjectName("cell");
    connect(cell, &QPushButton::clicked, this, [=,this]() {
        emit DispatcherUi::get()->ac_selected(ac_id);
    });
    cell->setFlat(true);
    QBoxLayout* lay = vertical ? static_cast<QBoxLayout*>(new QVBoxLayout(cell)) : static_cast<QBoxLayout*>(new QHBoxLayout(cell));
    lay->setContentsMargins(2, 0, 2, 0);
    lay->setSpacing(vertical ? 0 : 6);
    caption = new QLabel(cell);
    caption->setObjectName("caption");
    value = new QLabel(cell);
    value->setObjectName("value");
    for(auto l: {caption, value}) {
        l->setAttribute(Qt::WA_TransparentForMouseEvents);
        lay->addWidget(l);
    }
    if(vertical) {
        cell->setMinimumHeight(caption->sizeHint().height() + value->sizeHint().height() + 6);
    }
    cell->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    return cell;
}

/// The status icons that are not ok, with a short text.
void MiniStrip::updateStatusRow() {
    QStringList problems;
    for(auto l: {link_icon, gps_icon, rc_icon, imu_icon}) {
        bool show = l->property("problem").toBool();
        l->setVisible(show || all_icons);
        if(show) {
            problems.append(l->toolTip());
        }
    }
    status_text->setText(problems.join(" · "));
    status_row->setVisible((!problems.isEmpty() || all_icons) && !compact);
}

void MiniStrip::handle_bat_status(AircraftWatcher::BatStatus bs) {
    // the voltage in amber or red
    QString level;
    switch (bs) {
    case AircraftWatcher::BatStatus::CATASTROPHIC:
    case AircraftWatcher::BatStatus::CRITIC:
        level = "critic";
        break;
    case AircraftWatcher::BatStatus::LOW:
        level = "low";
        break;
    case AircraftWatcher::BatStatus::OK:
        break;
    }
    if(bat_label->property("bat").toString() != level) {
        bat_label->setProperty("bat", level);
        restyle(bat_label);
    }
}

void MiniStrip::updateFlightTime(uint32_t flight_time, uint32_t block_time, uint32_t stage_time) {
    uint32_t time = 0;
    switch (time_mode) {
    case FLIGHT_TIME:
        time = flight_time; break;
    case BLOCK_TIME:
        time = block_time; break;
    case STAGE_TIME:
        time = stage_time; break;

    }
    int hours = static_cast<int>(static_cast<int64_t>(time)/3600);
    int minutes = static_cast<int>(static_cast<int64_t>(time)/60 - hours*60);
    int seconds = static_cast<int>(static_cast<int64_t>(time) - minutes*60 -hours*3600);

    QString f_time = QString("%1").arg(hours, 2, 10, QChar('0')) + ":" +
                     QString("%1").arg(minutes, 2, 10, QChar('0')) + ":" +
                     QString("%1").arg(seconds, 2, 10, QChar('0'));
    flight_time_label->setText(f_time);
}

void MiniStrip::updateAp(QString ap_mode) {
    // a pill in the mode's color (gray while unknown)
    QColor color = QColor("#d0d7de");
    if(mode_colors.contains(ap_mode)) {
        color = QColor(mode_colors[ap_mode]);
    }
    QString style = QString("QPushButton { background: %1; color: black; font-size: 12px; font-weight: bold;"
                            " border: 0; border-radius: 4px; padding: 2px 8px; }").arg(color.name());
    if(ap_mode_button->styleSheet() != style) {
        ap_mode_button->setStyleSheet(style);
    }
    ap_mode_button->setText(ap_mode.isEmpty() ? QString::fromUtf8("\xE2\x80\x94") : ap_mode);
}

void MiniStrip::updateGps(QString gps_mode) {
    bool problem = true;
    if(gps_mode == "NOFIX" || gps_mode == "NA" || gps_mode == "2D" || gps_mode == "") {
        gps_icon->setPixmap(gps_lost.pixmap(30,30));
    } else if(gps_mode == "3D" ) {
        gps_icon->setPixmap(gps_ok.pixmap(30,30));
        problem = false;
    } else if(gps_mode == "DGPS") {
        gps_icon->setPixmap(gps_dgps.pixmap(30,30));
        problem = false;
    } else if(gps_mode == "RTK") {
        gps_icon->setPixmap(gps_rtk.pixmap(30,30));
        problem = false;
    }
    gps_icon->setToolTip("GPS " + (gps_mode.isEmpty() ? QString("?") : gps_mode));
    gps_icon->setProperty("problem", problem);
}

void MiniStrip::updateImu(QString state_filter_mode) {
    bool ok = state_filter_mode == "OK";
    imu_icon->setPixmap((ok ? imu_ok : imu_lost).pixmap(icons_size));
    imu_icon->setToolTip("IMU " + (state_filter_mode.isEmpty() ? QString("?") : state_filter_mode));
    imu_icon->setProperty("problem", !ok);
}

void MiniStrip::updateCurrentBlock(uint8_t cur_block) {
    shared_ptr<Block> block;
    try {
        block = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan()->getBlock(cur_block);
    } catch(std::runtime_error &e) {
        // a block that is not in the loaded flight plan (e.g. another build): do not stop the GCS
        block_label->setText(QString("Block %1").arg(cur_block));
        block_label->setToolTip(e.what());
        block_icon->hide();
        return;
    }
    QString block_name = block->getName();
    block_label->setToolTip("Block " + block_name);
    block_label->setText(block_name);

    QString icon_name = block->getIcon();
    if(icon_name != "") {
        QString icon_path = appConfig()->value("GCS_ICONS_PATH").toString() + "/" + icon_name;
        block_icon->setPixmap(QIcon(icon_path).pixmap(QSize(20, 20)));
        block_icon->show();
    } else {
        block_icon->hide();
    }
}

void MiniStrip::updateData() {

    // ENGINE_STATUS
    float bat = 0;
    float throttle = 0;

    // AP_STATUS
    QString kill_mode, ap_mode, gps_mode, state_filter_mode; // lat_mode, horiz_mode, gaz_mode;
    uint32_t flight_time = 0;

    // FLIGHT_PARAM
    float speed = 0;
    float alt = 0;
    float climb = 0;
    float agl = 0;
    float airspeed = 0;

    // FLY_BY_WIRE
    QString rc_status, rc_mode;

    // NAV_STATUS
    uint8_t cur_block = 0;
    float target_alt = 0;
    uint32_t block_time = 0;
    uint32_t stage_time = 0;


    auto engine_status_msg = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getMessage("ENGINE_STATUS");
    if(engine_status_msg) {
        engine_status_msg->getField("throttle", throttle);
        engine_status_msg->getField("bat", bat);
    }

    auto ap_status_msg = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getMessage("AP_STATUS");
    if(ap_status_msg) {
        ap_status_msg->getField("flight_time", flight_time);
        ap_status_msg->getField("kill_mode", kill_mode);
        ap_status_msg->getField("ap_mode", ap_mode);
        ap_status_msg->getField("gps_mode", gps_mode);
        ap_status_msg->getField("state_filter_mode", state_filter_mode);
    }

    auto flight_param_msg = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getMessage("FLIGHT_PARAM");
    if(flight_param_msg) {

        flight_param_msg->getField("speed", speed);
        flight_param_msg->getField("alt", alt);
        flight_param_msg->getField("climb", climb);
        flight_param_msg->getField("agl", agl);
        flight_param_msg->getField("airspeed", airspeed);
    }

    auto telemetry_messages = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getTelemetryMessages();

    auto fly_by_wire_msg = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getMessage("FLY_BY_WIRE");
    if(fly_by_wire_msg) {
        fly_by_wire_msg->getField("rc_status", rc_status);
        fly_by_wire_msg->getField("rc_mode", rc_mode);
    }

    auto nav_status_msg = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getMessage("NAV_STATUS");
    if(nav_status_msg) {
        nav_status_msg->getField("cur_block", cur_block);
        nav_status_msg->getField("target_alt", target_alt);
        nav_status_msg->getField("block_time", block_time);
        nav_status_msg->getField("stage_time", stage_time);
    }


    // battery
    if(bat_nb_cells != 0 && bat_per_cell) {
        bat_label->setText(QString("%1 V/c").arg(bat/bat_nb_cells, 0, 'f', 2));
    } else {
        bat_label->setText(QString("%1 V").arg(bat, 0, 'f', 1));
    }

    // throttle, and kill
    throttle_label->setText(QString("%1 %").arg(throttle, 0, 'f', 0));
    bool throttle_killed = !(kill_mode == "OFF" || kill_mode.isEmpty());
    throttle_caption->setText("Throttle");
    if(throttle_killed) {
        throttle_label->setText("KILLED");
    }
    if(throttle_label->property("killed").toBool() != throttle_killed) {
        throttle_label->setProperty("killed", throttle_killed);
        throttle_label->setStyleSheet(throttle_killed ? "color: #d92d20; font-weight: bold;" : "");
    }
    if(kill_button != nullptr) {
        bool killed = !(kill_mode == "OFF" || kill_mode.isEmpty());
        if(kill_button->property("killed").toBool() != killed || !kill_button->property("killed").isValid()) {
            kill_button->setProperty("killed", killed);
            // the state: green while the throttle is on, red while it is killed
            kill_button->setIcon(QIcon(user_or_app_path(killed ? "pictures/strip_kill.svg" : "pictures/strip_resurrect.svg")));
            kill_button->setToolTip(killed ? "Throttle killed (click: resurrect it, asks first)" :
                                             "Throttle on (click: kill it, asks first)");
            kill_button->style()->unpolish(kill_button);
            kill_button->style()->polish(kill_button);
        }
    }

    // flight time
    updateFlightTime(flight_time, block_time, stage_time);
    fitCells();

    // mode
    updateAp(ap_mode);

    // GPS
    updateGps(gps_mode);

    // IMU
    updateImu(state_filter_mode);

    // link
    int nb_links_ok = 0;
    for(auto &telemetry_msg: telemetry_messages) {
        float time_since_last_msg = 9999;
        telemetry_msg.getField("time_since_last_msg", time_since_last_msg);
        if(time_since_last_msg <= 5) {
            nb_links_ok += 1;
        }
    }

    QString link_state;
    if(nb_links_ok == telemetry_messages.size()) {
        link_icon->setPixmap(link_ok.pixmap(icons_size));
    } else if(nb_links_ok > 0) {
        link_icon->setPixmap(link_warning.pixmap(icons_size));
        link_state = "warning";
    } else {
        link_icon->setPixmap(link_lost.pixmap(icons_size));
        link_state = "lost";
    }
    if(property("link").toString() != link_state) {
        setProperty("link", link_state);
        restyle(this);
    }
    if(telemetry_messages.size() > 1) {
        link_icon->setToolTip(QString("Link %1/%2").arg(nb_links_ok).arg(telemetry_messages.size()));
    } else {
        link_icon->setToolTip(nb_links_ok > 0 ? "Link ok" : "Link lost");
    }
    link_icon->setProperty("problem", !link_state.isEmpty());

    // RC
    bool rc_is_ok = rc_status == "OK";
    rc_icon->setPixmap((rc_is_ok ? rc_ok : rc_lost).pixmap(icons_size));
    rc_icon->setToolTip("RC " + (rc_status.isEmpty() ? QString("?") : rc_status));
    rc_icon->setProperty("problem", !rc_is_ok);

    updateStatusRow();

    // alt
    double alt_display = alt_mode ? agl: alt;
    double diff_alt = alt - target_alt;
    QString alt_str = QString("%1 | %2%3 m")
            .arg(alt_display, 0, 'f', 0)
            .arg(diff_alt >= 0 ? "+": "")
            .arg(diff_alt, 0, 'f', abs(diff_alt) > 10 ? 0: 1);
    alt_label->setText(alt_str);

    // speed
    switch (speed_mode) {
    case GROUND_SPEED:
        speed_label->setText(QString::number(speed, 'f', 1) + " m/s");
        break;
    case AIR_SPEED:
        speed_label->setText(QString::number(airspeed, 'f', 1) + " m/s");
        break;
    case VERTICAL_SPEED:
        speed_label->setText(QString::number(climb, 'f', 1) + " m/s");
        break;
    }

    // current block
    updateCurrentBlock(cur_block);


//    QString txt = QString::number(climb, 'f', 1);
//    if(climb > 0) {
//        txt = "+" + txt;
//    }
//    full_alt_graph->setSecondayText(txt);
//    short_vspeed_label->setText(txt);
//    if(abs(speed) > 0.1) {
//        full_alt_graph->setIndicatorAngle((climb/speed)*1.2);
//    } else if(abs(climb) > 0.01){
//        full_alt_graph->setIndicatorAngle(climb/abs(climb)*0.3);
//    } else {
//        full_alt_graph->setIndicatorAngle(0);
//    }

//    if(climb > 0.5) {
//        short_vspeed_indicator->setText(QString::fromUtf8("\xE2\x86\x97"));
//    } else if(climb < -0.5) {
//        short_vspeed_indicator->setText(QString::fromUtf8("\xE2\x86\x98"));
//    } else {
//        short_vspeed_indicator->setText(QString::fromUtf8("\xE2\x86\x92"));
//    }


    emit updated();

}

void MiniStrip::mousePressEvent(QMouseEvent *event) {
    (void)event;
    emit DispatcherUi::get()->ac_selected(ac_id);
}

bool MiniStrip::eventFilter(QObject *object, QEvent *event)
{
    (void) object;
    // a double click on a value (they are buttons) also centers the map on the aircraft
    if (event->type() == QEvent::MouseButtonDblClick) {
        centerOnAircraft();
    }
    return false;
}

void MiniStrip::mouseDoubleClickEvent(QMouseEvent *event) {
    (void)event;
    centerOnAircraft();
}

void MiniStrip::centerOnAircraft() {
    emit DispatcherUi::get()->ac_selected(ac_id);
    if(AircraftManager::get()->aircraftExists(ac_id)) {
        auto ac = AircraftManager::get()->getAircraft(ac_id);
        if(ac->isReal()) {
            emit DispatcherUi::get()->centerMap(ac->getPosition());
        }
    }
}

void MiniStrip::setCompact(bool c) {
    // two lines: the values without captions; the status icons (not ok) on the first line
    compact = c;
    for(auto l: captions) {
        l->setVisible(!compact);
    }
    // the time without its caption: no empty space before it (room for the block name)
    if(time_button->property("full_width").isValid()) {
        time_button->setMinimumWidth(compact ? flight_time_label->sizeHint().width() + 6 :
                                               time_button->property("full_width").toInt());
    }
    for(auto cell: value_cells) {
        if(!cell->property("full_height").isValid()) {
            cell->setProperty("full_height", cell->minimumHeight());
        }
        cell->setMinimumHeight(compact ? 0 : cell->property("full_height").toInt());
    }
    int i = 0;
    for(auto icon: {link_icon, gps_icon, rc_icon, imu_icon}) {
        icon->setFixedSize(compact ? QSize(16, 16) : QSize(22, 22));
        if(compact) {
            compact_icons->addWidget(icon);
        } else {
            status_layout->insertWidget(i++, icon);
        }
    }
    if(auto lay = qobject_cast<QVBoxLayout*>(layout())) {
        lay->setContentsMargins(8, compact ? 3 : 6, 8, compact ? 3 : 6);
        lay->setSpacing(compact ? 0 : 4);
    }
    updateStatusRow();
    fitCells();
}

/// The value cells not narrower than their caption and value (they were cut off when the panel
/// is made narrow); a button does not take the size of its layout by itself.
void MiniStrip::fitCells() {
    for(auto cell: value_cells) {
        if(cell->layout()) {
            cell->layout()->activate();
            cell->setMinimumWidth(cell->layout()->minimumSize().width());
        }
    }
}
