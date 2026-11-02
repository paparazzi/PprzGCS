#include "pfd.h"
#include "gcs_utils.h"
#include <QContextMenuEvent>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <math.h>
#include <cmath>
#include <AircraftManager.h>
#include "dispatcher_ui.h"

// colors of the indicator (the same in both themes: it is an instrument)
static const QColor SKY_TOP(0x2f7fd1), SKY_BOTTOM(0x7cb8ee);
static const QColor GROUND_TOP(0x9a6a3a), GROUND_BOTTOM(0x6b4423);
static const QColor SYMBOL(0xffc400);

Pfd::Pfd(QWidget *parent) : QWidget(parent), current_ac("")
{
    earth_view = getAppSettings().value("pfd/ground_view", false).toBool();
    setMinimumSize(minimumSizeHint());     // the tapes and the horizon need room (also in a splitter)
    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this, &Pfd::changeCurrentAC);
    connect(PprzDispatcher::get(), &PprzDispatcher::flight_param, this, &Pfd::updateEulers);

    // The attitude straight from the telemetry: at its own rate, instead of the rate of the
    // server's FLIGHT_PARAM (about 1 Hz)
    PprzDispatcher::get()->bindOptional("ATTITUDE", this, [=,this](QString sender, pprzlink::Message msg) {
        float phi, theta, psi;
        msg.getField("phi", phi);
        msg.getField("theta", theta);
        msg.getField("psi", psi);
        const float r2d = 180.f / static_cast<float>(M_PI);
        updateAttitude(sender, phi * r2d, theta * r2d, psi * r2d);
    });
    PprzDispatcher::get()->bindOptional("ROTORCRAFT_FP", this, [=,this](QString sender, pprzlink::Message msg) {
        int32_t phi, theta, psi;
        msg.getField("phi", phi);
        msg.getField("theta", theta);
        msg.getField("psi", psi);
        const float coef = 0.0139882f;   // ANGLE_BFP to deg
        updateAttitude(sender, phi * coef, theta * coef, psi * coef);
    });
    setToolTip("Attitude (roll, pitch) and heading of the selected aircraft (right click: pilot or ground view)");
}

void Pfd::paintEvent(QPaintEvent *event) {
    (void)event;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    bool valid = eulers.find(current_ac) != eulers.end();
    Eulers e = valid ? eulers[current_ac] : Eulers{0, 0, 0};
    QColor ac_color = QColor(0x808080);
    if(valid && AircraftManager::get()->aircraftExists(current_ac)) {
        ac_color = AircraftManager::get()->getAircraft(current_ac)->getColor();
    }

    // square attitude indicator, the heading tape below
    // attitude, with the height tape on its right; heading tape and values below
    // speed tape | attitude | height tape; heading tape and values below
    const double tape_h = 34, values_h = 22, alt_w = 58, spd_w = 54, gap = 6;
    double side = qMin<double>(width() - alt_w - spd_w - 2 * gap, height() - tape_h - values_h - 2 * gap);
    double left = (width() - side - alt_w - spd_w - 2 * gap) / 2;
    QRectF spd_tape(left, 0, spd_w, side);
    QRectF att(spd_tape.right() + gap, 0, side, side);
    QRectF alt_tape(att.right() + gap, 0, alt_w, side);
    QRectF tape(att.left(), att.bottom() + gap, side, tape_h);
    QRectF values(spd_tape.left(), tape.bottom() + gap, spd_w + side + alt_w + 2 * gap, values_h);

    paintSpeed(p, spd_tape, e, valid);
    paintAttitude(p, att, e.roll, e.pitch, ac_color, valid);
    paintAltitude(p, alt_tape, e, valid);
    paintHeading(p, tape, e.yaw, valid);

    // the values as text below the instrument
    QFont vf = font();
    vf.setPixelSize(13);
    p.setFont(vf);
    QColor text = palette().color(QPalette::WindowText);
    QColor secondary = text;
    secondary.setAlpha(170);
    auto value = [&](QRectF r, QString caption, QString v) {
        p.setPen(secondary);
        p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, caption);
        QFont bf = vf;
        bf.setBold(true);
        p.setFont(bf);
        p.setPen(text);
        p.drawText(r, Qt::AlignRight | Qt::AlignVCenter, v);
        p.setFont(vf);
    };
    QString dash = QString::fromUtf8("\xE2\x80\x94");
    double third = values.width() / 4;
    value(QRectF(values.left(), values.top(), third - 10, values_h), "Roll", valid ? QString("%1°").arg(e.roll, 0, 'f', 0) : dash);
    value(QRectF(values.left() + third + 5, values.top(), third - 10, values_h), "Pitch", valid ? QString("%1°").arg(e.pitch, 0, 'f', 0) : dash);
    double hd = fmod(fmod(e.yaw, 360.) + 360., 360.);
    value(QRectF(values.left() + 2 * third + 10, values.top(), third - 10, values_h), "Hdg", valid ? QString("%1°").arg(static_cast<int>(round(hd)) % 360) : dash);
    value(QRectF(values.left() + 3 * third + 15, values.top(), third - 15, values_h), "MSL", valid ? QString("%1").arg(e.alt, 0, 'f', 0) : dash);
}

void Pfd::paintAttitude(QPainter& p, QRectF r, float roll, float pitch, QColor ac_color, bool valid) {
    const double radius = 10;
    const double px_per_deg = r.height() / 60.;    // +-30 deg visible
    QPointF c = r.center();

    p.save();
    QPainterPath clip;
    clip.addRoundedRect(r, radius, radius);
    p.setClipPath(clip);

    // pilot view: the horizon turns with the roll and moves with the pitch (the aircraft fixed);
    // ground view: the horizon fixed, the aircraft symbol banks and pitches (seen from the ground)
    p.save();
    p.translate(c);
    if(!earth_view) {
        p.rotate(-roll);
        p.translate(0, pitch * px_per_deg);
    }
    // sky and ground: past the corners for any pitch (e.g. a tailsitter at -90 deg)
    double big = std::abs(pitch) * px_per_deg + std::hypot(r.width(), r.height()) + 10;
    QLinearGradient sky(0, -big / 2, 0, 0);
    sky.setColorAt(0, SKY_TOP);
    sky.setColorAt(1, SKY_BOTTOM);
    QLinearGradient ground(0, 0, 0, big / 2);
    ground.setColorAt(0, GROUND_TOP);
    ground.setColorAt(1, GROUND_BOTTOM);
    p.setPen(Qt::NoPen);
    p.setBrush(sky);
    p.drawRect(QRectF(-big, -big, 2 * big, big));
    p.setBrush(ground);
    p.drawRect(QRectF(-big, 0, 2 * big, big));
    p.setPen(QPen(Qt::white, 2));
    p.drawLine(QPointF(-big, 0), QPointF(big, 0));

    // pitch ladder every 5 deg, labels every 10
    QFont f = font();
    f.setPixelSize(qMax(9, static_cast<int>(r.height() / 22)));
    f.setBold(true);
    p.setFont(f);
    for(int deg = -60; deg <= 60; deg += 5) {
        if(deg == 0) {
            continue;
        }
        double y = -deg * px_per_deg;
        double w = (deg % 10 == 0) ? r.width() * 0.22 : r.width() * 0.10;
        p.setPen(QPen(Qt::white, deg % 10 == 0 ? 2 : 1.4));
        p.drawLine(QPointF(-w / 2, y), QPointF(w / 2, y));
        if(deg % 10 == 0) {
            QString txt = QString::number(abs(deg));
            QRectF lr(w / 2 + 4, y - 8, 30, 16);
            p.drawText(lr, Qt::AlignVCenter | Qt::AlignLeft, txt);
            QRectF rr(-w / 2 - 34, y - 8, 30, 16);
            p.drawText(rr, Qt::AlignVCenter | Qt::AlignRight, txt);
        }
    }
    p.restore();

    // roll scale: ticks at the top, fixed; the pointer turns with the roll
    double rr = r.width() * 0.42;
    p.save();
    p.translate(c);
    p.setPen(QPen(Qt::white, 2));
    for(int a: {-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60}) {
        p.save();
        p.rotate(a);
        double len = (a % 30 == 0) ? 12 : 7;
        p.drawLine(QPointF(0, -rr), QPointF(0, -rr - len));
        p.restore();
    }
    // the pointer: with the horizon (pilot view) or with the aircraft (ground view)
    p.rotate(earth_view ? roll : -roll);
    QPolygonF pointer;
    pointer << QPointF(0, -rr + 1) << QPointF(-7, -rr + 12) << QPointF(7, -rr + 12);
    p.setPen(Qt::NoPen);
    p.setBrush(SYMBOL);
    p.drawPolygon(pointer);
    p.restore();

    // the aircraft symbol: fixed (pilot view), or banked and pitched (ground view)
    p.save();
    p.translate(c);
    if(earth_view) {
        double lim = qMax(0.0, r.height() / 2 - 10);
        double dy = qBound(-lim, -pitch * px_per_deg, lim);
        p.translate(0, dy);
        p.rotate(roll);
    }
    double s = r.width() * 0.18;
    QPainterPath wings;
    wings.moveTo(-s * 1.6, 0);
    wings.lineTo(-s * 0.55, 0);
    wings.lineTo(-s * 0.3, s * 0.3);
    wings.moveTo(s * 1.6, 0);
    wings.lineTo(s * 0.55, 0);
    wings.lineTo(s * 0.3, s * 0.3);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(Qt::black, 7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(wings);
    p.setPen(QPen(SYMBOL, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(wings);
    p.setPen(QPen(Qt::black, 1.5));
    p.setBrush(SYMBOL);
    p.drawEllipse(QPointF(0, 0), 4, 4);
    p.restore();

    // values: roll and pitch as text
    QFont vf = font();
    vf.setPixelSize(qMax(10, static_cast<int>(r.height() / 18)));
    vf.setBold(true);
    p.setFont(vf);
    auto chip = [&](QRectF cr, QString txt) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 120));
        p.drawRoundedRect(cr, 5, 5);
        p.setPen(Qt::white);
        p.drawText(cr, Qt::AlignCenter, txt);
    };
    double ch = vf.pixelSize() + 10;
    if(!valid) {
        chip(QRectF(c.x() - r.width() * 0.3, r.bottom() - ch - 8, r.width() * 0.6, ch), "No attitude data");
    }
    p.restore();

    // border in the aircraft color
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(ac_color, 3));
    p.drawRoundedRect(r.adjusted(1.5, 1.5, -1.5, -1.5), radius, radius);
}

void Pfd::paintHeading(QPainter& p, QRectF r, float heading, bool valid) {
    const double radius = 8;
    const double px_per_deg = r.width() / 90.;    // 90 deg visible
    p.save();
    QPainterPath clip;
    clip.addRoundedRect(r, radius, radius);
    p.setClipPath(clip);
    p.fillRect(r, QColor(0x1b1f25));

    QFont f = font();
    f.setPixelSize(11);
    f.setBold(true);
    p.setFont(f);
    double hd = fmod(fmod(heading, 360.) + 360., 360.);
    double cx = r.center().x();
    for(int d = static_cast<int>(hd) - 60; d <= static_cast<int>(hd) + 60; d++) {
        if(d % 5 != 0) {
            continue;
        }
        double x = cx + (d - hd) * px_per_deg;
        int dn = ((d % 360) + 360) % 360;
        p.setPen(QPen(QColor(0xd6dce3), 1.4));
        double len = (dn % 10 == 0) ? 8 : 4;
        p.drawLine(QPointF(x, r.top()), QPointF(x, r.top() + len));
        if(dn % 30 == 0) {
            QString txt;
            switch(dn) {
            case 0: txt = "N"; break;
            case 90: txt = "E"; break;
            case 180: txt = "S"; break;
            case 270: txt = "W"; break;
            default: txt = QString::number(dn / 10);
            }
            p.drawText(QRectF(x - 15, r.top() + 10, 30, r.height() - 12), Qt::AlignHCenter | Qt::AlignVCenter, txt);
        }
    }
    p.restore();

    // the heading in a box in the middle
    QRectF box(cx - 28, r.top() + 4, 56, r.height() - 8);
    p.setPen(QPen(SYMBOL, 2));
    p.setBrush(QColor(0x1b1f25));
    p.drawRoundedRect(box, 5, 5);
    p.setPen(Qt::white);
    QFont bf = font();
    bf.setPixelSize(14);
    bf.setBold(true);
    p.setFont(bf);
    p.drawText(box, Qt::AlignCenter, valid ? QString("%1°").arg(static_cast<int>(round(hd)) % 360, 3, 10, QChar('0')) : "---");
}

/**
 * Height above ground: a vertical tape (+-25 m visible) with the value in a box, a marker at the
 * target height of the flight plan and the climb rate below.
 */
void Pfd::paintAltitude(QPainter& p, QRectF r, const Eulers& e, bool valid) {
    const double radius = 8;
    const double range = 50.;      // m visible
    double px_per_m = r.height() / range;
    double cy = r.center().y();
    p.save();
    QPainterPath clip;
    clip.addRoundedRect(r, radius, radius);
    p.setClipPath(clip);
    p.fillRect(r, QColor(0x1b1f25));

    QFont f = font();
    f.setPixelSize(10);
    f.setBold(true);
    p.setFont(f);
    double h = valid ? e.agl : 0;
    for(int m = static_cast<int>(floor((h - range / 2) / 5)) * 5; m <= h + range / 2; m += 5) {
        double y = cy - (m - h) * px_per_m;
        p.setPen(QPen(m < 0 ? QColor(0xb07a4a) : QColor(0xd6dce3), 1.4));
        double len = (m % 10 == 0) ? 9 : 5;
        p.drawLine(QPointF(r.left(), y), QPointF(r.left() + len, y));
        // no numbers under the caption (top) and the climb (bottom)
        if(m % 10 == 0 && y > r.top() + 22 && y < r.bottom() - 22) {
            p.drawText(QRectF(r.left() + 12, y - 7, r.width() - 14, 14), Qt::AlignLeft | Qt::AlignVCenter, QString::number(m));
        }
    }
    // the ground
    double gy = cy + h * px_per_m;
    if(gy < r.bottom()) {
        p.fillRect(QRectF(r.left(), gy, r.width(), r.bottom() - gy), QColor(107, 68, 35, 150));
    }
    // the target height of the flight plan
    if(valid && AircraftManager::get()->aircraftExists(current_ac)) {
        auto nav = AircraftManager::get()->getAircraft(current_ac)->getStatus()->getMessage("NAV_STATUS");
        if(nav) {
            float target_alt = 0;
            nav->getField("target_alt", target_alt);
            double target_agl = target_alt - (e.alt - e.agl);
            double ty = qBound(r.top() + 4, cy - (target_agl - h) * px_per_m, qMax(r.top() + 4, r.bottom() - 4));
            QPolygonF tri;
            tri << QPointF(r.right(), ty - 6) << QPointF(r.right() - 9, ty) << QPointF(r.right(), ty + 6);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0x4fd1ff));
            p.drawPolygon(tri);
        }
    }
    p.restore();

    // the height in a box
    QRectF box(r.left() + 2, cy - 13, r.width() - 4, 26);
    p.setPen(QPen(SYMBOL, 2));
    p.setBrush(QColor(0x1b1f25));
    p.drawRoundedRect(box, 5, 5);
    QFont bf = font();
    bf.setPixelSize(14);
    bf.setBold(true);
    p.setFont(bf);
    p.setPen(Qt::white);
    p.drawText(box, Qt::AlignCenter, valid ? QString::number(e.agl, 'f', 0) : "---");

    // caption and climb
    QFont cf = font();
    cf.setPixelSize(10);
    cf.setBold(true);
    p.setFont(cf);
    p.setPen(QColor(0xd6dce3));
    p.drawText(QRectF(r.left(), r.top() + 2, r.width(), 14), Qt::AlignCenter, "AGL m");
    QString climb = valid ? QString("%1%2").arg(e.climb > 0.05 ? QString::fromUtf8("\xE2\x86\x91") : (e.climb < -0.05 ? QString::fromUtf8("\xE2\x86\x93") : QString()))
                                           .arg(fabs(e.climb), 0, 'f', 1) : QString();
    p.drawText(QRectF(r.left(), r.bottom() - 16, r.width(), 14), Qt::AlignCenter, climb);
}

/// Ground speed: a vertical tape (+-10 m/s visible) with the value in a box, airspeed below.
void Pfd::paintSpeed(QPainter& p, QRectF r, const Eulers& e, bool valid) {
    const double radius = 8;
    const double range = 20.;      // m/s visible
    double px_per_ms = r.height() / range;
    double cy = r.center().y();
    double v = valid ? e.speed : 0;
    p.save();
    QPainterPath clip;
    clip.addRoundedRect(r, radius, radius);
    p.setClipPath(clip);
    p.fillRect(r, QColor(0x1b1f25));
    QFont f = font();
    f.setPixelSize(10);
    f.setBold(true);
    p.setFont(f);
    for(int s = qMax(0, static_cast<int>(floor(v - range / 2))); s <= v + range / 2; s++) {
        double y = cy - (s - v) * px_per_ms;
        p.setPen(QPen(QColor(0xd6dce3), 1.4));
        double len = (s % 5 == 0) ? 9 : 4;
        p.drawLine(QPointF(r.right() - len, y), QPointF(r.right(), y));
        if(s % 5 == 0 && y > r.top() + 22 && y < r.bottom() - 22) {
            p.drawText(QRectF(r.left() + 2, y - 7, r.width() - 14, 14), Qt::AlignRight | Qt::AlignVCenter, QString::number(s));
        }
    }
    p.restore();

    QRectF box(r.left() + 2, cy - 13, r.width() - 4, 26);
    p.setPen(QPen(SYMBOL, 2));
    p.setBrush(QColor(0x1b1f25));
    p.drawRoundedRect(box, 5, 5);
    QFont bf = font();
    bf.setPixelSize(14);
    bf.setBold(true);
    p.setFont(bf);
    p.setPen(Qt::white);
    p.drawText(box, Qt::AlignCenter, valid ? QString::number(e.speed, 'f', 1) : "---");

    QFont cf = font();
    cf.setPixelSize(10);
    cf.setBold(true);
    p.setFont(cf);
    p.setPen(QColor(0xd6dce3));
    p.drawText(QRectF(r.left(), r.top() + 2, r.width(), 14), Qt::AlignCenter, "GS m/s");
    if(valid && e.airspeed > 0.1) {
        p.drawText(QRectF(r.left(), r.bottom() - 16, r.width(), 14), Qt::AlignCenter, QString("AS %1").arg(e.airspeed, 0, 'f', 1));
    }
}

void Pfd::changeCurrentAC(QString id) {
    if(AircraftManager::get()->aircraftExists(id)) {
        current_ac = id;
        update();
    }
}

void Pfd::updateEulers(pprzlink::Message msg) {
    QString ac_id;
    float roll, pitch, heading;
    msg.getField("ac_id", ac_id);
    if(AircraftManager::get()->aircraftExists(ac_id)) {
        msg.getField("roll", roll);
        msg.getField("pitch", pitch);
        msg.getField("heading", heading);
        if(!std::isfinite(roll) || !std::isfinite(pitch) || !std::isfinite(heading)) {
            return;     // keep the last good attitude
        }
        Eulers e {roll, pitch, heading};
        // the attitude from the telemetry is newer (when it came in the last 2 s)
        if(direct_attitude.count(ac_id) && direct_attitude[ac_id].isValid() &&
           direct_attitude[ac_id].elapsed() < 2000 && eulers.count(ac_id)) {
            e.roll = eulers[ac_id].roll;
            e.pitch = eulers[ac_id].pitch;
            e.yaw = eulers[ac_id].yaw;
        }
        msg.getField("agl", e.agl);
        msg.getField("alt", e.alt);
        msg.getField("climb", e.climb);
        msg.getField("speed", e.speed);
        msg.getField("airspeed", e.airspeed);
        eulers[ac_id] = e;
        if(ac_id == current_ac) {
            update();
        }
    }
}

void Pfd::updateAttitude(QString ac_id, float roll, float pitch, float yaw) {
    if(!AircraftManager::get()->aircraftExists(ac_id) ||
       !std::isfinite(roll) || !std::isfinite(pitch) || !std::isfinite(yaw)) {
        return;
    }
    if(!eulers.count(ac_id)) {
        return;     // the rest (speeds, heights) comes with the first FLIGHT_PARAM
    }
    auto &e = eulers[ac_id];
    e.roll = roll;
    e.pitch = pitch;
    e.yaw = fmod(fmod(yaw, 360.f) + 360.f, 360.f);
    direct_attitude[ac_id].start();
    if(ac_id == current_ac) {
        update();
    }
}

void Pfd::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    auto pilot = menu.addAction("Pilot view (the horizon moves)");
    auto ground = menu.addAction("Ground view (the aircraft moves)");
    pilot->setCheckable(true);
    ground->setCheckable(true);
    pilot->setChecked(!earth_view);
    ground->setChecked(earth_view);
    auto chosen = menu.exec(event->globalPos());
    if(chosen == pilot || chosen == ground) {
        earth_view = chosen == ground;
        auto settings = getAppSettings();
        settings.setValue("pfd/ground_view", earth_view);
        update();
    }
}

QSize Pfd::sizeHint() const
{
    return QSize(380, 320);
}

QSize Pfd::minimumSizeHint() const
{
    return QSize(320, 260);
}
