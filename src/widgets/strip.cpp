#include "strip.h"
#include "AircraftManager.h"
#include <QPainter>
#include <QPaintEvent>

Strip::Strip(QString ac_id, QWidget *parent) : QWidget(parent), _ac_id(ac_id)
{
    layout = new QVBoxLayout(this);
    lay_bat_link = new QVBoxLayout();
    lay_head = new QHBoxLayout();
    lay_body = new QHBoxLayout();
    //lay_status = new QVBoxLayout();

    layout->addLayout(lay_head);
    layout->addLayout(lay_body);

    lay_body->addLayout(lay_bat_link);
    //lay_body->addLayout(lay_status);

    flight_time_label = new QLabel("00:00:00", this);
    flight_time_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lay_head->addWidget(flight_time_label);

    speed_label = new JaugeLabel(0, 10, "m/s", this);
    speed_label->setPrecision(1);
    speed_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lay_head->addWidget(speed_label);
    speed_label->setStatus(true);

    throttle_label = new JaugeLabel(0, 100, "%", this);
    throttle_label->setPrecision(0);
    throttle_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lay_head->addWidget(throttle_label);


    // get bat min and max
    //AircraftManager::get()->getAircraft(ac_id).getAirframe()
    bat_graph = new GraphLabel(8, 14, this);
    bat_graph->setUnit("V");
    lay_bat_link->addWidget(bat_graph);

    link_label = new ColorLabel(this);
    link_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lay_bat_link->addWidget(link_label);

    QGroupBox* gp = new QGroupBox(this);
    gp->setTitle("status");
    lay_body->addWidget(gp);
    QVBoxLayout* status_layout = new QVBoxLayout(gp);

    ap_mode_label = new ColorLabel(this);
    ap_mode_label->setMinSize(QSize(60, 50));
    ap_mode_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ap_mode_label->setToolTip("Navigation mode");
    status_layout->addWidget(ap_mode_label);

    fbw_mode_label = new ColorLabel(this);
    fbw_mode_label->setMinSize(QSize(60, 50));
    fbw_mode_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    fbw_mode_label->setToolTip("Radio Command status");
    status_layout->addWidget(fbw_mode_label);


    gps_mode_label = new ColorLabel(this);
    gps_mode_label->setMinSize(QSize(60, 50));
    gps_mode_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    gps_mode_label->setToolTip("GPS status");
    status_layout->addWidget(gps_mode_label);

    alt_graph = new GraphLabel(0, 120, this);
    alt_graph->setDualText(true);
    alt_graph->setSecondayText("+0.0");
    alt_graph->setPrecision(0);
    alt_graph->setUnit("m");
    alt_graph->setIndicator(true);
    lay_body->addWidget(alt_graph);


    connect(PprzDispatcher::get(), &PprzDispatcher::engine_status, this, &Strip::updateEngineStatus);
    connect(PprzDispatcher::get(), &PprzDispatcher::ap_status, this, &Strip::updateApStatus);
    connect(PprzDispatcher::get(), &PprzDispatcher::flight_param, this, &Strip::updateFlightParams);
    connect(PprzDispatcher::get(), &PprzDispatcher::telemetry_status, this, &Strip::updateTelemetryStatus);
    connect(PprzDispatcher::get(), &PprzDispatcher::fly_by_wire, this, &Strip::updateFBW);
}


void Strip::paintEvent(QPaintEvent* e) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRect(e->rect());
    p.setPen(Qt::NoPen);

    QColor color = AircraftManager::get()->getAircraft(_ac_id).getColor();
    int hue = color.hue();
    int sat = color.saturation();
    color.setHsv(hue, static_cast<int>(sat*0.2), 255);

    p.fillPath(path, color);
    p.drawPath(path);

    QWidget::paintEvent(e);
}

void Strip::updateEngineStatus(pprzlink::Message msg) {
    std::string id;
    msg.getField("ac_id", id);
    if(id.c_str() == _ac_id) {
        float bat, throttle;
        msg.getField("throttle", throttle);
        msg.getField("bat", bat);
        bat_graph->pushData(bat);
        throttle_label->setValue(throttle);
    }
}

void Strip::updateApStatus(pprzlink::Message msg) {
    std::string id;
    msg.getField("ac_id", id);
    if(id.c_str() == _ac_id) {
        string ap_mode, lat_mode, horiz_mode, gaz_mode, gps_mode, kill_mode;
        uint32_t flight_time;

        msg.getField("flight_time", flight_time);
        msg.getField("kill_mode", kill_mode);
        msg.getField("ap_mode", ap_mode);
        msg.getField("gps_mode", gps_mode);

        int hours = static_cast<int>(static_cast<int64_t>(flight_time)/3600);
        int minutes = static_cast<int>(static_cast<int64_t>(flight_time)/60 - hours*60);
        int seconds = static_cast<int>(static_cast<int64_t>(flight_time) - minutes*60 -hours*3600);

        QString f_time = QString("%1").arg(hours, 2, 10, QChar('0')) + ":" +
                         QString("%1").arg(minutes, 2, 10, QChar('0')) + ":" +
                         QString("%1").arg(seconds, 2, 10, QChar('0'));
        flight_time_label->setText(f_time);

        throttle_label->setStatus(kill_mode=="OFF");

        if(ap_mode == "HOME" || ap_mode == "FAILSAFE") {
            ap_mode_label->setBrush(Qt::red);
        } else if (ap_mode == "MANUAL") {
            ap_mode_label->setBrush(QColor("#ffa500"));
        } else {
            ap_mode_label->setBrush(QColor("#7ef17e"));
        }
        ap_mode_label->setText(ap_mode.c_str());


        if(gps_mode == "NOFIX") {
            gps_mode_label->setBrush(Qt::red);
        } else if (gps_mode == "NA" || gps_mode == "2D") {
            gps_mode_label->setBrush(QColor("#ffa500"));
        } else {
            gps_mode_label->setBrush(QColor("#7ef17e"));
        }
        gps_mode_label->setText(gps_mode.c_str());

    }
}

void Strip::updateFlightParams(pprzlink::Message msg) {
    std::string id;
    msg.getField("ac_id", id);
    if(id.c_str() == _ac_id) {
        float speed, alt, climb, agl, airspeed;
        msg.getField("speed", speed);
        msg.getField("alt", alt);
        msg.getField("climb", climb);
        msg.getField("agl", agl);
        msg.getField("airspeed", airspeed);
        speed_label->setValue(speed);
        alt_graph->pushData(agl);
        QString txt = QString::number(climb, 'f', 1);
        if(climb > 0) {
            txt = "+" + txt;
        }
        alt_graph->setSecondayText(txt);
        if(abs(speed) > 0.1) {
            alt_graph->setIndicatorAngle((climb/speed)*1.2);
        } else if(abs(climb) > 0.01){
            alt_graph->setIndicatorAngle(climb/abs(climb)*0.3);
        } else {
            alt_graph->setIndicatorAngle(0);
        }

    }
}


void Strip::updateTelemetryStatus(pprzlink::Message msg) {
    std::string id;
    msg.getField("ac_id", id);
    if(id.c_str() == _ac_id) {
        float time_since_last_msg;
        msg.getField("time_since_last_msg", time_since_last_msg);
        if(time_since_last_msg > 5) {
            link_label->setBrush(Qt::red);
        } else {
            link_label->setBrush(QColor("#7ef17e"));
        }

        if(time_since_last_msg > 2) {
            link_label->setText(QString::number(time_since_last_msg, 'f', 0));
        } else {
            link_label->setText("");
        }
    }
}

void Strip::updateFBW(pprzlink::Message msg) {
    std::string id;
    msg.getField("ac_id", id);
    if(id.c_str() == _ac_id) {
        std::string rc_status, rc_mode;
        msg.getField("rc_status", rc_status);
        msg.getField("rc_mode", rc_mode);

        if(rc_status == "OK") {
            fbw_mode_label->setBrush(QColor("#7ef17e"));
        } else if (rc_status == "LOST" || rc_status == "REALLY_LOST") {
            fbw_mode_label->setBrush(Qt::red);
        } else {
            fbw_mode_label->setBrush(QColor("#ffa500"));
        }

        fbw_mode_label->setText(rc_status.c_str());

    }
}
