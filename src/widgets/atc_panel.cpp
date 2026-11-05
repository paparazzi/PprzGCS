#include "atc_panel.h"
#include "gcs_utils.h"
#include "dispatcher_ui.h"
#include <AircraftManager.h>
#include "flightplan.h"
#include "waypoint.h"
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <cmath>

static constexpr double FT = 3.28084;       // ft per m
static constexpr double KT = 1.94384;       // kt per m/s
static constexpr double NM = 1852.;         // m per NM
static constexpr double STD_P = 101325.;    // Pa

// pressure altitude (ft) above the level of the reference pressure ref (Pa), standard atmosphere
static double pressureAltFt(double p, double ref) {
    return 145366.45 * (1. - std::pow(p / ref, 0.190284));
}

static const char* REF_NAMES[3] = {"QNH", "QFE", "FL"};

AtcPanel::AtcPanel(QWidget *parent) : QWidget(parent)
{
    auto settings = getAppSettings();
    alt_ref = static_cast<AltRef>(qBound(0, settings.value("atc/alt_ref", QNH).toInt(), 2));
    qnh_set = settings.value("atc/qnh", 0).toFloat();

    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this, [=,this](QString id) {
        current_ac = id;
        update();
    });

    PprzDispatcher::get()->bindOptional("AIR_DATA", this, [=,this](QString sender, pprzlink::Message msg) {
        auto& ad = air_data[sender];
        msg.getField("pressure", ad.pressure);
        msg.getField("qnh", ad.qnh);
        ad.last.start();
        // own QNH estimate, for aircraft that don't send one: the QNH for which the pressure
        // altitude is the GPS altitude (MSL), low-pass filtered
        if(ad.pressure > 0 && AircraftManager::get()->aircraftExists(sender)) {
            auto fp = AircraftManager::get()->getAircraft(sender)->getStatus()->getMessage("FLIGHT_PARAM");
            if(fp) {
                float alt_msl;
                fp->getField("alt", alt_msl);
                double q = ad.pressure / std::pow(1. - alt_msl * FT / 145366.45, 1. / 0.190284) / 100.;
                if(q > 800 && q < 1100) {
                    ad.qnh_est = ad.qnh_est > 0 ? ad.qnh_est + 0.05 * (q - ad.qnh_est) : q;
                }
            }
        }
    });
    PprzDispatcher::get()->bindOptional("INS_REF", this, [=,this](QString sender, pprzlink::Message msg) {
        msg.getField("baro_qfe", air_data[sender].qfe);
    });

    auto t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &AtcPanel::tick);
    t->start(250);

    setToolTip("Click the altitude reference to change it; right click: set the QNH");
}

QSize AtcPanel::sizeHint() const {
    return QSize(300, 500);
}

QSize AtcPanel::minimumSizeHint() const {
    return QSize(180, 300);
}

void AtcPanel::tick() {
    // the timer distance: the last ground speed, also when the link is lost
    qint64 now = timer.isValid() ? timer.elapsed() : 0;
    if(timer_on) {
        timer_dist += last_speed * (now - last_tick_ms) / 1000.;
        last_tick_ms = now;
    }
    if(AircraftManager::get()->aircraftExists(current_ac)) {
        auto fp = AircraftManager::get()->getAircraft(current_ac)->getStatus()->getMessage("FLIGHT_PARAM");
        if(fp) {
            fp->getField("speed", last_speed);
        }
    }
    update();
}

void AtcPanel::setAltRef(AltRef ref) {
    alt_ref = ref;
    getAppSettings().setValue("atc/alt_ref", static_cast<int>(ref));
    update();
}

void AtcPanel::askQnh() {
    bool ok;
    double v = QInputDialog::getDouble(this, "QNH", "QNH (hPa), 0: the one of the aircraft",
                                       qnh_set, 0, 1100, 0, &ok);
    if(ok) {
        qnh_set = v;
        getAppSettings().setValue("atc/qnh", qnh_set);
        update();
    }
}

void AtcPanel::mousePressEvent(QMouseEvent *event) {
    if(event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    if(timer_button.contains(event->position())) {
        if(timer_on) {
            timer_on = false;   // reset
        } else {
            timer_on = true;
            timer.start();
            last_tick_ms = 0;
            timer_dist = 0;
        }
        update();
        return;
    }
    for(int i=0; i<3; i++) {
        if(ref_chips[i].contains(event->position())) {
            setAltRef(static_cast<AltRef>(i));
            return;
        }
    }
}

void AtcPanel::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    for(int i=0; i<3; i++) {
        auto a = menu.addAction(QString("Altitude on %1").arg(REF_NAMES[i]));
        a->setCheckable(true);
        a->setChecked(alt_ref == i);
        connect(a, &QAction::triggered, this, [=,this]() {setAltRef(static_cast<AltRef>(i));});
    }
    menu.addSeparator();
    connect(menu.addAction("Set QNH..."), &QAction::triggered, this, &AtcPanel::askQnh);
    menu.exec(event->globalPos());
}

void AtcPanel::paintEvent(QPaintEvent *event) {
    (void)event;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const QColor text = palette().color(QPalette::WindowText);
    QColor caption = text;
    caption.setAlphaF(0.6);
    QColor line = text;
    line.setAlphaF(0.2);
    const QColor accent(0xf0a020);
    const QColor red(0xc62828);

    // the layout is made for 420 x 700, scaled to the widget
    const double s = qMin(width() / 420., height() / 700.);
    const double m = 16 * s;
    const double w = width();
    auto font = [&](double px, bool mono) {
        QFont f = mono ? QFont("monospace") : this->font();
        if(mono) f.setStyleHint(QFont::TypeWriter);
        f.setBold(true);
        f.setPixelSize(qMax(1, int(px * s)));
        return f;
    };

    // values ("---" without data)
    QString name = "---", heading = "---", speed = "---", alt = "---", alt_unit = "ft", alt_info, time = "---", bat;
    QString home = "---";
    QString ref_name = "HOME";      // ARP waypoint if the flight plan has one
    bool have_ac = AircraftManager::get()->aircraftExists(current_ac);
    if(have_ac) {
        auto ac = AircraftManager::get()->getAircraft(current_ac);
        name = ac->name();
        auto status = ac->getStatus();
        auto fp = status->getMessage("FLIGHT_PARAM");
        if(fp) {
            float course, gspeed, alt_msl, agl;
            double lat, lon;
            fp->getField("course", course);
            fp->getField("speed", gspeed);
            fp->getField("alt", alt_msl);
            fp->getField("agl", agl);
            fp->getField("lat", lat);
            fp->getField("long", lon);
            int hdg = (int)std::lround(course / 5.f) * 5;
            hdg = ((hdg % 360) + 360) % 360;
            heading = QString("%1").arg(hdg, 3, 10, QChar('0'));
            speed = QString::number(std::lround(gspeed * KT));

            // altitude: from the pressure when the aircraft sends it, else from FLIGHT_PARAM
            AirData ad;
            if(air_data.count(current_ac)) ad = air_data[current_ac];
            bool baro = ad.last.isValid() && ad.last.elapsed() < 5000 && ad.pressure > 0;
            double qnh = STD_P / 100.;
            QString qnh_src = "std";
            if(qnh_set > 0) {
                qnh = qnh_set; qnh_src = "set";
            } else if(ad.qnh > 0) {
                qnh = ad.qnh; qnh_src = "aircraft";
            } else if(ad.qnh_est > 0) {
                qnh = ad.qnh_est; qnh_src = "estimated";
            }
            double ft = NAN;
            switch(alt_ref) {
            case QNH:
                ft = baro ? pressureAltFt(ad.pressure, qnh * 100.) : alt_msl * FT;
                alt_info = baro ? QString("QNH %1 hPa, %2").arg(qnh, 0, 'f', 0).arg(qnh_src) : "GPS altitude (MSL)";
                break;
            case QFE:
                if(baro && ad.qfe > 0) {
                    ft = pressureAltFt(ad.pressure, ad.qfe);
                    alt_info = QString("QFE %1 hPa").arg(ad.qfe / 100., 0, 'f', 0);
                } else {
                    ft = agl * FT;
                    alt_info = "above ground, GPS";
                }
                break;
            case FL:
                if(baro) {
                    ft = pressureAltFt(ad.pressure, STD_P);
                    alt_info = QString("%1 ft on 1013 hPa").arg(std::lround(ft / 10.) * 10);
                } else {
                    alt_info = "no pressure";
                }
                break;
            }
            if(!std::isnan(ft) && alt_ref == FL) {
                // flight level: hundreds of ft on the standard pressure, 3 digits
                alt = QString("FL%1").arg(qMax(0L, std::lround(ft / 100.)), 3, 10, QChar('0'));
                alt_unit = "";
            } else if(!std::isnan(ft)) {
                alt = QString::number(std::lround(ft / 10.) * 10);   // what is said on the radio
            }

            // bearing and distance from HOME
            auto fpl = ac->getFlightPlan();
            auto find_wp = [fpl](QString n) -> Waypoint* {
                if(fpl) {
                    for(auto w: fpl->getWaypoints()) {
                        if(w->getName() == n) return w;
                    }
                }
                return nullptr;
            };
            Waypoint* wp = find_wp("ARP");
            if(wp) {
                ref_name = "ARP";
            } else {
                wp = find_wp("HOME");
            }
            if(wp) {
                double dn = (lat - wp->getLat()) * M_PI / 180. * 6371000.;
                double de = (lon - wp->getLon()) * M_PI / 180. * 6371000. * std::cos(wp->getLat() * M_PI / 180.);
                double d = std::hypot(dn, de);
                int brg = (int)std::lround(std::atan2(de, dn) * 180. / M_PI / 10.) * 10;
                brg = ((brg % 360) + 360) % 360;
                home = QString("bearing %1, %2 NM").arg(brg, 3, 10, QChar('0')).arg(d / NM, 0, 'f', 1);
            }
        }
        auto ap = status->getMessage("AP_STATUS");
        if(ap) {
            uint32_t ft;
            ap->getField("flight_time", ft);
            time = QString::number(ft / 60);
        }
        auto eng = status->getMessage("ENGINE_STATUS");
        if(eng) {
            float v;
            eng->getField("bat", v);
            auto vmax = ac->getAirframe()->getDefine("MAX_BAT_LEVEL");
            auto vmin = ac->getAirframe()->getDefine("CATASTROPHIC_BAT_LEVEL");
            if(vmax && vmin && vmax->value.toFloat() > vmin->value.toFloat()) {
                float pct = 100.f * (v - vmin->value.toFloat()) / (vmax->value.toFloat() - vmin->value.toFloat());
                bat = QString("BAT %1 %").arg(std::lround(qBound(0.f, pct, 100.f)));
            } else {
                bat = QString("BAT %1 V").arg(v, 0, 'f', 1);
            }
        }
    }

    double y = 12 * s;
    p.setPen(accent);
    p.setFont(font(18, false));
    p.drawText(QPointF(m, y + 18 * s), "ATC");
    p.setPen(text);
    p.setFont(font(44, true));
    p.drawText(QRectF(m, y + 28 * s, w - 2 * m, 56 * s), Qt::AlignLeft | Qt::AlignVCenter, name);
    y = 100 * s;

    // one row: caption, big value, unit; returns the row rectangle
    auto row = [&](const QString& cap, const QString& value, const QString& unit, double h = 104) {
        QRectF r(m, y, w - 2 * m, h * s);
        p.setPen(line);
        p.drawLine(QPointF(m, y), QPointF(w - m, y));
        p.setPen(caption);
        p.setFont(font(16, false));
        p.drawText(QPointF(m, y + 24 * s), cap);
        p.setPen(text);
        auto vf = font(64, true);
        p.setFont(vf);
        p.drawText(QPointF(m, y + 88 * s), value);
        double vw = QFontMetricsF(vf).horizontalAdvance(value);
        p.setFont(font(26, false));
        p.drawText(QPointF(m + vw + 8 * s, y + 88 * s), unit);
        y += h * s;
        return r;
    };

    row("HEADING (track)", heading, "");
    row("SPEED (ground)", speed, "kt");
    QRectF alt_row = row("ALTITUDE", alt, alt_unit);

    // altitude reference chips
    {
        auto cf = font(16, false);
        p.setFont(cf);
        QFontMetricsF fm(cf);
        double x = w - m;
        for(int i=2; i>=0; i--) {
            double tw = fm.horizontalAdvance(REF_NAMES[i]) + 12 * s;
            x -= tw;
            QRectF chip(x, alt_row.top() + 6 * s, tw, 22 * s);
            ref_chips[i] = chip;
            bool on = alt_ref == i;
            p.setPen(Qt::NoPen);
            p.setBrush(on ? accent : line);
            p.drawRoundedRect(chip, 5 * s, 5 * s);
            p.setPen(on ? QColor(Qt::black) : caption);
            p.drawText(chip, Qt::AlignCenter, REF_NAMES[i]);
            x -= 6 * s;
        }
        p.setBrush(Qt::NoBrush);
        p.setPen(caption);
        p.setFont(font(14, false));
        p.drawText(QRectF(m, alt_row.top(), w - 2 * m, alt_row.height() - 8 * s),
                   Qt::AlignRight | Qt::AlignBottom, alt_info);
    }

    QRectF time_row = row("FLIGHT TIME", time, "min");
    p.setPen(text);
    p.setFont(font(22, false));
    p.drawText(QRectF(m, time_row.top(), w - 2 * m, time_row.height() - 14 * s),
               Qt::AlignRight | Qt::AlignBottom, bat);

    // timer
    p.setPen(line);
    p.drawLine(QPointF(m, y), QPointF(w - m, y));
    timer_button = QRectF(m, y + 12 * s, 154 * s, 40 * s);
    p.setPen(Qt::NoPen);
    p.setBrush(timer_on ? line : red);
    p.drawRoundedRect(timer_button, 8 * s, 8 * s);
    p.setBrush(Qt::NoBrush);
    p.setPen(timer_on ? text : QColor(Qt::white));
    p.setFont(font(18, false));
    p.drawText(timer_button, Qt::AlignCenter, timer_on ? "RESET TIMER" : "START TIMER");
    QString since = "--:--", dist = "~ -- NM";
    if(timer_on) {
        qint64 sec = timer.elapsed() / 1000;
        since = QString("%1:%2").arg(sec / 60, 2, 10, QChar('0')).arg(sec % 60, 2, 10, QChar('0'));
        dist = QString("~ %1 NM").arg(timer_dist / NM, 0, 'f', 1);
    }
    p.setPen(caption);
    p.setFont(font(14, false));
    p.drawText(QPointF(m + 174 * s, y + 24 * s), "since");
    p.setPen(timer_on ? accent : caption);
    p.setFont(font(40, true));
    p.drawText(QPointF(m + 174 * s, y + 62 * s), since);
    p.drawText(QPointF(m, y + 104 * s), dist);
    y += 120 * s;

    p.setPen(line);
    p.drawLine(QPointF(m, y), QPointF(w - m, y));
    p.setPen(caption);
    p.setFont(font(16, false));
    p.drawText(QPointF(m, y + 24 * s), "FROM " + ref_name);
    p.setPen(text);
    p.setFont(font(24, false));
    p.drawText(QPointF(m, y + 54 * s), home);
}
