#include "mini_strip.h"
#include "AircraftManager.h"
#include "gcs_utils.h"
#include "dispatcher_ui.h"
#include "mini_strip.h"
#include <QDebug>
#include <QSettings>

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
    ac_id(ac_id), icons_size(QSize(30, 30)), alt_mode(true), speed_mode(SpeedMode::GROUND_SPEED)
{
    auto gl = new QGridLayout(this);
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    auto ac_name = new QLabel(ac->name());
    ac_name->setStyleSheet("font-weight: bold");
    gl->addWidget(ac_name, 0, 0);


    /////////// alt ///////////
    QHBoxLayout* alt_layout = new QHBoxLayout();
    alt_button = new QToolButton(this);
    alt_button->setIconSize(icons_size);
    agl_icon = QIcon(":/pictures/agl.svg");
    msl_icon = QIcon(":/pictures/msl.svg");
    alt_button->setIcon(agl_icon);
    alt_layout->addWidget(alt_button);
    alt_label = new QLabel("0 m", this);
    alt_label->setToolTip("Altitude AGL");
    alt_layout->addWidget(alt_label);
    gl->addLayout(alt_layout, 1, 1);
    connect(alt_button, &QToolButton::clicked, this,
        [=]() {
            alt_mode = !alt_mode;
            if(alt_mode) {
                alt_button->setIcon(agl_icon);
                alt_label->setToolTip("Altitude AGL");
            } else {
                alt_button->setIcon(msl_icon);
                alt_label->setToolTip("Altitude MSL");
            }
            updateData();
        });

    /////////// flight time ///////////
    QHBoxLayout* ft_lay = new QHBoxLayout();
    auto ft_icon = new QLabel(this);
    auto ft_pix = QIcon(":/pictures/flight_time.svg").pixmap(icons_size);
    ft_icon->setPixmap(ft_pix);
    ft_icon->setToolTip("Flight time");
    ft_lay->addWidget(ft_icon);
    flight_time_label = new QLabel("0 s", this);
    flight_time_label->setToolTip("Flight time");
    ft_lay->addWidget(flight_time_label);
    ft_lay->addStretch();
    gl->addLayout(ft_lay, 0, 1);


    /////////// speed //////////////////
    QHBoxLayout* speed_lay = new QHBoxLayout();
    speed_button = new QToolButton(this);
    speed_button->setIconSize(icons_size);
    ground_speed_icon = QIcon(":/pictures/ground_speed.svg");
    air_speed_icon = QIcon(":/pictures/airspeed.svg");
    vertical_speed_icon = QIcon(":/pictures/vertical_speed.svg");
    speed_button->setIcon(ground_speed_icon);
    speed_lay->addWidget(speed_button);
    speed_label = new QLabel("0 m/s", this);
    speed_label->setToolTip("Ground speed");
    speed_lay->addWidget(speed_label);
    gl->addLayout(speed_lay, 2, 1);
    connect(speed_button, &QToolButton::clicked, this,
        [=]() {
            switch (speed_mode) {
            case GROUND_SPEED:
                speed_mode = AIR_SPEED;
                speed_button->setIcon(air_speed_icon);
                speed_label->setToolTip("Air speed");
                break;
            case AIR_SPEED:
                speed_mode = VERTICAL_SPEED;
                speed_button->setIcon(vertical_speed_icon);
                speed_label->setToolTip("Vertical speed");
                break;
            case VERTICAL_SPEED:
                speed_mode = GROUND_SPEED;
                speed_button->setIcon(ground_speed_icon);
                speed_label->setToolTip("Ground speed");
                break;
            }
            updateData();
        });



    /////////// block /////////
    auto block_lay = new QHBoxLayout();
    block_icon = new QLabel(this);
    block_label = new QLabel(this);
    block_lay->addWidget(block_icon);
    block_lay->addWidget(block_label);
    block_lay->addStretch();
    block_icon->setToolTip("block");
    block_label->setToolTip("block");
    gl->addLayout(block_lay, 0, 2);

    ////////// bat /////////////
    auto bat_lay = new QHBoxLayout();
    bat_full = QIcon(":/pictures/bat_full.svg");
    bat_half = QIcon(":/pictures/bat_half.svg");
    bat_low = QIcon(":/pictures/bat_empty.svg");
    bat_icon = new QLabel(this);
    bat_icon->setPixmap(bat_low.pixmap(icons_size));
    bat_label = new QLabel("00.0 V", this);
    bat_lay->addWidget(bat_icon);
    bat_lay->addWidget(bat_label);
    gl->addLayout(bat_lay, 1, 0);


    ////////// throttle /////////////
    auto th_lay = new QHBoxLayout();
    throttle_on = QIcon(":/pictures/throttle_on.svg");
    throttle_killed = QIcon(":/pictures/throttle_killed.svg");
    throttle_icon = new QLabel(this);
    throttle_icon->setPixmap(throttle_killed.pixmap(icons_size));
    ft_icon->setToolTip("Flight Time");
    th_lay->addWidget(throttle_icon);
    throttle_label = new QLabel("0 %", this);
    throttle_label->setToolTip("Throttle");
    th_lay->addWidget(throttle_label);
    gl->addLayout(th_lay, 2, 0);

    //////// mode //////////
    ap_mode_label = new ColorLabel(10, this);
    gl->addWidget(ap_mode_label, 1, 2);
    ap_mode_label->setToolTip("AP mode");


    ///// Link, GPS, RC //////
    auto misc_lay = new QHBoxLayout();
    link_icon = new QLabel(this);
    link_ok = QIcon(":/pictures/link_ok");
    link_lost = QIcon(":/pictures/link_nok");
    link_icon->setPixmap(link_lost.pixmap(icons_size));
    link_icon->setToolTip("Link");
    misc_lay->addWidget(link_icon);

    gps_icon = new QLabel(this);
    gps_ok = QIcon(":/pictures/gps_ok.svg");
    gps_lost = QIcon(":/pictures/gps_nok.svg");
    gps_icon->setPixmap(gps_lost.pixmap(30,30));
    gps_icon->setToolTip("Gps");
    misc_lay->addWidget(gps_icon);


    rc_icon = new QLabel(this);
    rc_ok = QIcon(":/pictures/rc_ok.svg");
    rc_lost = QIcon(":/pictures/rc_nok.svg");
    rc_icon->setPixmap(rc_lost.pixmap(icons_size));
    rc_icon->setToolTip("RC");
    misc_lay->addWidget(rc_icon);

    imu_icon = new QLabel(this);
    imu_ok = QIcon(":/pictures/imu_ok.svg");
    imu_lost = QIcon(":/pictures/imu_nok.svg");
    imu_icon->setPixmap(imu_lost.pixmap(icons_size));
    imu_icon->setToolTip("IMU");
    misc_lay->addWidget(imu_icon);

    gl->addLayout(misc_lay, 2, 2);

    auto ac_status = AircraftManager::get()->getAircraft(ac_id)->getStatus();

    connect(ac_status, &AircraftStatus::engine_status, this, &MiniStrip::updateData);
    connect(ac_status, &AircraftStatus::flight_param, this, &MiniStrip::updateData);
    connect(ac_status, &AircraftStatus::telemetry_status, this, &MiniStrip::updateData);
    connect(ac_status, &AircraftStatus::fly_by_wire, this, &MiniStrip::updateData);
    connect(ac_status, &AircraftStatus::ap_status, this, &MiniStrip::updateData);
    connect(ac_status, &AircraftStatus::nav_status, this, &MiniStrip::updateData);
}

void MiniStrip::updateFlightTime(uint32_t flight_time) {
    int hours = static_cast<int>(static_cast<int64_t>(flight_time)/3600);
    int minutes = static_cast<int>(static_cast<int64_t>(flight_time)/60 - hours*60);
    int seconds = static_cast<int>(static_cast<int64_t>(flight_time) - minutes*60 -hours*3600);

    QString f_time = QString("%1").arg(hours, 2, 10, QChar('0')) + ":" +
                     QString("%1").arg(minutes, 2, 10, QChar('0')) + ":" +
                     QString("%1").arg(seconds, 2, 10, QChar('0'));
    flight_time_label->setText(f_time);
}

void MiniStrip::updateAp(QString ap_mode) {
    if(mode_colors.contains(ap_mode)) {
        ap_mode_label->setBrush(QColor(mode_colors[ap_mode]));
    } else {
        ap_mode_label->setBrush(Qt::white);
    }
    ap_mode_label->setText(ap_mode);
}

void MiniStrip::updateGps(QString gps_mode) {
    if(gps_mode == "NOFIX" || gps_mode == "NA" || gps_mode == "2D") {
        gps_icon->setPixmap(gps_lost.pixmap(30,30));
    } else {
        gps_icon->setPixmap(gps_ok.pixmap(30,30));
    }
    gps_icon->setToolTip("Gps " + QString(gps_mode));
}

void MiniStrip::updateImu(QString state_filter_mode) {
    if(state_filter_mode == "OK") {
        imu_icon->setPixmap(imu_ok.pixmap(icons_size));
    } else {
        imu_icon->setPixmap(imu_lost.pixmap(icons_size));
    }
    imu_icon->setToolTip("IMU " + QString(state_filter_mode));
}

void MiniStrip::updateCurrentBlock(uint8_t cur_block) {
    auto block = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan()->getBlock(cur_block);
    QString block_name = block->getName();
    block_label->setToolTip("Block " + block_name);
    if(block_name.size() > 15) {
        block_name.truncate(12);
        block_name += "...";
    }
    block_label->setText(block_name);

    auto settings = getAppSettings();

    QString icon_name = block->getIcon();
    if(icon_name != "") {
        QString icon_path = settings.value("path/gcs_icons").toString() + "/" + icon_name;
        block_icon->setPixmap(QIcon(icon_path).pixmap(icons_size));
    } else {
        block_icon->setText(QString::fromUtf8("\xE2\x88\x85"));
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

    // TELEMETRY_STATUS
    float time_since_last_msg = 99999;

    // FLY_BY_WIRE
    QString rc_status, rc_mode;

    // NAV_STATUS
    uint8_t cur_block = 0;
    float target_alt = 0;

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

    auto telemetry_status_msg = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getMessage("TELEMETRY_STATUS");
    if(telemetry_status_msg) {
        telemetry_status_msg->getField("time_since_last_msg", time_since_last_msg);
    }

    auto fly_by_wire_msg = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getMessage("FLY_BY_WIRE");
    if(fly_by_wire_msg) {
        fly_by_wire_msg->getField("rc_status", rc_status);
        fly_by_wire_msg->getField("rc_mode", rc_mode);
    }

    auto nav_status_msg = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getMessage("NAV_STATUS");
    if(nav_status_msg) {
        nav_status_msg->getField("cur_block", cur_block);
        nav_status_msg->getField("target_alt", target_alt);
    }


    // batterie
    bat_label->setText(QString::number(bat) + " V");

    // throttle
    /// TODO: change bat icon : full/half/low
    throttle_label->setText(QString::number(throttle) + " %");

    // kill
    if(kill_mode == "OFF") {
        throttle_icon->setPixmap(throttle_on.pixmap(icons_size));
    } else {
        throttle_icon->setPixmap(throttle_killed.pixmap(icons_size));
    }

    // flight time
    updateFlightTime(flight_time);


    // mode
    updateAp(ap_mode);

    // GPS
    updateGps(gps_mode);

    // IMU
    updateImu(state_filter_mode);

    // link
    if(time_since_last_msg > 5) {
        link_icon->setPixmap(link_lost.pixmap(icons_size));
        link_icon->setToolTip("Link lost since " + QString::number(static_cast<int>(time_since_last_msg)) + " s");
    } else {
        link_icon->setPixmap(link_ok.pixmap(icons_size));
        link_icon->setToolTip("Link");
    }

    // RC
    if(rc_status == "OK") {
        rc_icon->setPixmap(rc_ok.pixmap(icons_size));
    } else {
        rc_icon->setPixmap(rc_lost.pixmap(icons_size));
    }
    rc_icon->setToolTip("RC " + rc_status);

    // alt
    double alt_display = alt_mode ? agl: alt;
    double diff_alt = alt - target_alt;
    QString alt_str = QString("%1m | %2%3 m")
            .arg(alt_display, 0, 'f', 0)
            .arg(diff_alt > 0 ? "+": "")
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

bool MiniStrip::eventFilter(QObject *object, QEvent *event)
{
    (void) object;
    if (event->type() == QEvent::MouseButtonPress) {
        emit DispatcherUi::get()->ac_selected(ac_id);
        return true;
    }
    return false;
}
