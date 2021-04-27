#include "mini_strip.h"
#include "AircraftManager.h"
#include "gcs_utils.h"
#include "dispatcher_ui.h"
#include <QDebug>
#include <QSettings>

MiniStrip::MiniStrip(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id), icons_size(QSize(30, 30)), alt_mode(true), speed_mode(true)
{
    auto hl = new QHBoxLayout(this);
    auto gl = new QGridLayout();
    hl->setSizeConstraint(QLayout::SetFixedSize);

    auto ac = AircraftManager::get()->getAircraft(ac_id);
    QColor color = ac.getColor();
    int hue = color.hue();
    int sat = color.saturation();
    color.setHsv(hue, static_cast<int>(sat*0.2), 255);


    auto color_rect = new QWidget(this);
    color_rect->setMinimumSize(QSize(20, 20));
    color_rect->setStyleSheet("background: " + ac.getColor().name());

    color_rect->installEventFilter(this);

    hl->addWidget(color_rect);
    hl->addLayout(gl);

    auto ac_name = new QLabel(ac.name());
    ac_name->setStyleSheet("font-weight: bold");
//    auto ac_name = new ColorLabel(15, this);
//    ac_name->setText(ac.name());
//    ac_name->setBrush(color);
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
            updateFlightParams();
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
    gl->addLayout(ft_lay, 0, 1);


    /////////// speed //////////////////
    QHBoxLayout* speed_lay = new QHBoxLayout();
    speed_button = new QToolButton(this);
    speed_button->setIconSize(icons_size);
    ground_speed_icon = QIcon(":/pictures/ground_speed.svg");
    air_speed_icon = QIcon(":/pictures/airspeed.svg");
    speed_button->setIcon(ground_speed_icon);
    speed_lay->addWidget(speed_button);
    speed_label = new QLabel("0 m/s", this);
    speed_label->setToolTip("Ground speed");
    speed_lay->addWidget(speed_label);
    gl->addLayout(speed_lay, 2, 1);
    connect(speed_button, &QToolButton::clicked, this,
        [=]() {
            speed_mode = !speed_mode;
            if(speed_mode) {
                speed_button->setIcon(ground_speed_icon);
                speed_label->setToolTip("Ground speed");
            } else {
                speed_button->setIcon(air_speed_icon);
                speed_label->setToolTip("Air speed");
            }
            updateFlightParams();
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


    connect(AircraftManager::get()->getAircraft(ac_id).getStatus(),
            &AircraftStatus::engine_status, this, &MiniStrip::updateEngineStatus);
    connect(AircraftManager::get()->getAircraft(ac_id).getStatus(),
            &AircraftStatus::flight_param, this, &MiniStrip::updateFlightParams);
    connect(AircraftManager::get()->getAircraft(ac_id).getStatus(),
            &AircraftStatus::telemetry_status, this, &MiniStrip::updateTelemetryStatus);
    connect(AircraftManager::get()->getAircraft(ac_id).getStatus(),
            &AircraftStatus::fly_by_wire, this, &MiniStrip::updateFBW);
    connect(AircraftManager::get()->getAircraft(ac_id).getStatus(),
            &AircraftStatus::ap_status, this, &MiniStrip::updateApStatus);
    connect(AircraftManager::get()->getAircraft(ac_id).getStatus(),
            &AircraftStatus::nav_status, this, &MiniStrip::updateNavStatus);

    emit updated();
}

void MiniStrip::updateEngineStatus() {
    auto msg = AircraftManager::get()->getAircraft(ac_id).getStatus()->getMessage("ENGINE_STATUS");
    if(msg) {
        float bat, throttle;
        msg->getField("throttle", throttle);
        msg->getField("bat", bat);
        bat_label->setText(QString::number(bat) + " V");
        /// TODO: change bat icon : full/half/low
        throttle_label->setText(QString::number(throttle) + " %");
        emit updated();
    }
}

void MiniStrip::updateApStatus() {
    auto msg = AircraftManager::get()->getAircraft(ac_id).getStatus()->getMessage("AP_STATUS");
    if(msg) {
        QString kill_mode, ap_mode, gps_mode, state_filter_mode; // lat_mode, horiz_mode, gaz_mode;
        uint32_t flight_time;

        msg->getField("flight_time", flight_time);
        msg->getField("kill_mode", kill_mode);
        msg->getField("ap_mode", ap_mode);
        msg->getField("gps_mode", gps_mode);
        msg->getField("state_filter_mode", state_filter_mode);

        int hours = static_cast<int>(static_cast<int64_t>(flight_time)/3600);
        int minutes = static_cast<int>(static_cast<int64_t>(flight_time)/60 - hours*60);
        int seconds = static_cast<int>(static_cast<int64_t>(flight_time) - minutes*60 -hours*3600);

        QString f_time = QString("%1").arg(hours, 2, 10, QChar('0')) + ":" +
                         QString("%1").arg(minutes, 2, 10, QChar('0')) + ":" +
                         QString("%1").arg(seconds, 2, 10, QChar('0'));
        flight_time_label->setText(f_time);

        if(kill_mode == "OFF") {
            throttle_icon->setPixmap(throttle_on.pixmap(icons_size));
        } else {
            throttle_icon->setPixmap(throttle_killed.pixmap(icons_size));
        }

        if(ap_mode == "HOME" || ap_mode == "FAILSAFE") {
            ap_mode_label->setBrush(Qt::red);
        } else if (ap_mode == "MANUAL") {
            ap_mode_label->setBrush(QColor(0xffa500));
        } else {
            ap_mode_label->setBrush(QColor(0x7ef17e));
        }
        ap_mode_label->setText(ap_mode);


        if(gps_mode == "NOFIX" || gps_mode == "NA" || gps_mode == "2D") {
            gps_icon->setPixmap(gps_lost.pixmap(30,30));
        } else {
            gps_icon->setPixmap(gps_ok.pixmap(30,30));
        }
        gps_icon->setToolTip("Gps " + QString(gps_mode));

        if(state_filter_mode == "OK") {
            imu_icon->setPixmap(imu_ok.pixmap(icons_size));
        } else {
            imu_icon->setPixmap(imu_lost.pixmap(icons_size));
        }
        imu_icon->setToolTip("IMU " + QString(state_filter_mode));
        emit updated();
    }
}

void MiniStrip::updateFlightParams() {
    auto msg = AircraftManager::get()->getAircraft(ac_id).getStatus()->getMessage("FLIGHT_PARAM");
    if(msg) {
        float speed, alt, climb, agl, airspeed;
        msg->getField("speed", speed);
        msg->getField("alt", alt);
        msg->getField("climb", climb);
        msg->getField("agl", agl);
        msg->getField("airspeed", airspeed);

        if(alt_mode) {
            alt_label->setText(QString::number(agl, 'f', 0) + " m");
        } else {
            alt_label->setText(QString::number(alt, 'f', 0) + " m");
        }

        if(speed_mode) {
            speed_label->setText(QString::number(speed, 'f', 1) + " m/s");
        } else {
            speed_label->setText(QString::number(airspeed, 'f', 1) + " m/s");
        }

        emit updated();

//        QString txt = QString::number(climb, 'f', 1);
//        if(climb > 0) {
//            txt = "+" + txt;
//        }
//        full_alt_graph->setSecondayText(txt);
//        short_vspeed_label->setText(txt);
//        if(abs(speed) > 0.1) {
//            full_alt_graph->setIndicatorAngle((climb/speed)*1.2);
//        } else if(abs(climb) > 0.01){
//            full_alt_graph->setIndicatorAngle(climb/abs(climb)*0.3);
//        } else {
//            full_alt_graph->setIndicatorAngle(0);
//        }

//        if(climb > 0.5) {
//            short_vspeed_indicator->setText(QString::fromUtf8("\xE2\x86\x97"));
//        } else if(climb < -0.5) {
//            short_vspeed_indicator->setText(QString::fromUtf8("\xE2\x86\x98"));
//        } else {
//            short_vspeed_indicator->setText(QString::fromUtf8("\xE2\x86\x92"));
//        }

    }
}


void MiniStrip::updateTelemetryStatus() {
    auto msg = AircraftManager::get()->getAircraft(ac_id).getStatus()->getMessage("TELEMETRY_STATUS");
    if(msg) {
        float time_since_last_msg;
        msg->getField("time_since_last_msg", time_since_last_msg);
        if(time_since_last_msg > 5) {
            link_icon->setPixmap(link_lost.pixmap(icons_size));
            link_icon->setToolTip("Link lost since " + QString::number(static_cast<int>(time_since_last_msg)) + " s");
        } else {
            link_icon->setPixmap(link_ok.pixmap(icons_size));
            link_icon->setToolTip("Link");
        }
        emit updated();
    }
}

void MiniStrip::updateFBW() {
    auto msg = AircraftManager::get()->getAircraft(ac_id).getStatus()->getMessage("FLY_BY_WIRE");
    if(msg) {
        QString rc_status, rc_mode;
        msg->getField("rc_status", rc_status);
        msg->getField("rc_mode", rc_mode);

        if(rc_status == "OK") {
            rc_icon->setPixmap(rc_ok.pixmap(icons_size));
        } else {
            rc_icon->setPixmap(rc_lost.pixmap(icons_size));
        }

        rc_icon->setToolTip("RC " + rc_status);
        emit updated();
    }
}

void MiniStrip::updateNavStatus() {
    QSettings settings(qApp->property("SETTINGS_PATH").toString(), QSettings::IniFormat);
    auto msg = AircraftManager::get()->getAircraft(ac_id).getStatus()->getMessage("NAV_STATUS");
    if(msg) {
        uint8_t cur_block;
        msg->getField("cur_block", cur_block);
        auto block = AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getBlock(cur_block);
        QString block_name = block->getName();
        block_label->setToolTip("Block " + block_name);
        if(block_name.size() > 15) {
            block_name.truncate(12);
            block_name += "...";
        }
        block_label->setText(block_name);


        QString icon_name = block->getIcon();
        if(icon_name != "") {
            QString icon_path = settings.value("path/gcs_icons").toString() + "/" + icon_name;
            block_icon->setPixmap(QIcon(icon_path).pixmap(icons_size));
        } else {
            block_icon->setText(QString::fromUtf8("\xE2\x88\x85"));
        }
    }
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
