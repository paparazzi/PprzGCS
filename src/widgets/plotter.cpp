#include "plotter.h"

#include <QtWidgets>
#include "AircraftManager.h"
#include "gcs_utils.h"
#include <QDebug>

namespace {
const qint64 KEEP_MS = 61 * 60 * 1000;      // the values kept: the longest time window (60 min), and a bit
const QColor BLUE(0x1d5fc9), GREEN(0x1f7a3f), ORANGE(0xb54708), RED(0xb42318), GREY(0x8c959f),
             PURPLE(0x6639ba), MAGENTA(0xbf3989);

const QString BAT = "ground:ENGINE_STATUS:bat";
const QString AMP = "ground:ENGINE_STATUS:amp";
const QString THROTTLE = "ground:ENGINE_STATUS:throttle";
const QString POWER = "ground:ENGINE_STATUS:power";         // bat * amp
const QString ENERGY = "ground:ENGINE_STATUS:charge_mah";   // charge (Ah) in mAh
const QString SPEED = "ground:FLIGHT_PARAM:speed";
const QString AIRSPEED = "ground:FLIGHT_PARAM:airspeed";
const QString AGL = "ground:FLIGHT_PARAM:agl";
const QString TARGET_AGL = "ground:NAV_STATUS:target_agl";  // target_alt (MSL) above the ground
const QString ROLL = "attitude:roll", PITCH = "attitude:pitch", YAW = "attitude:yaw";     // deg

/// The telemetry messages received from each aircraft (for "Other message field"): one listener
/// for all the plotters
QHash<QString, QSet<QString>>& seenTelemetry() {
    static QHash<QString, QSet<QString>> seen;
    static bool bound = false;
    if(!bound) {
        bound = true;
        auto dict = PprzDispatcher::get()->getDict();
        for(auto &def: dict->getMsgsForClass("telemetry")) {
            QString name = def.getName();
            PprzDispatcher::get()->bind(name, qApp, [name](QString sender, pprzlink::Message msg) {
                (void)msg;
                seen[sender].insert(name);
            });
        }
    }
    return seen;
}

/// A numeric field of a message (an element of an array field with index >= 0)
std::optional<double> fieldValue(pprzlink::Message& msg, const QString& field, int index) {
    try {
        auto t = msg.getDefinition().getField(field).getType();
        auto bt = t.getBaseType();
        auto get = [&](auto zero) -> double {
            using T = decltype(zero);
            if(t.isArray()) {
                std::vector<T> values;
                msg.getField(field, values);
                return index >= 0 && index < static_cast<int>(values.size()) ? static_cast<double>(values[index]) : NAN;
            }
            T v;
            msg.getField(field, v);
            return static_cast<double>(v);
        };
        double v = NAN;
        switch(bt) {
        case pprzlink::BaseType::FLOAT: v = get(float()); break;
        case pprzlink::BaseType::DOUBLE: v = get(double()); break;
        case pprzlink::BaseType::UINT8: v = get(uint8_t()); break;
        case pprzlink::BaseType::UINT16: v = get(uint16_t()); break;
        case pprzlink::BaseType::UINT32: v = get(uint32_t()); break;
        case pprzlink::BaseType::INT8: v = get(int8_t()); break;
        case pprzlink::BaseType::INT16: v = get(int16_t()); break;
        case pprzlink::BaseType::INT32: v = get(int32_t()); break;
        default: return std::nullopt;
        }
        if(std::isfinite(v)) {
            return v;
        }
    } catch(std::exception&) {
    }
    return std::nullopt;
}
}

/////////////////////////////// PlotChart ///////////////////////////////

PlotChart::PlotChart(Plotter* plotter, PlotDef def, QWidget* parent) : QWidget(parent),
    plotter(plotter), def(def)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

double PlotChart::timeAt(double x) const {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    return now - plotter->window() + x / qMax(1, width()) * plotter->window();
}

void PlotChart::mouseMoveEvent(QMouseEvent* e) {
    emit hovered(static_cast<qint64>(timeAt(e->position().x())));
}

void PlotChart::leaveEvent(QEvent*) {
    emit hovered(-1);
}

void PlotChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r = QRectF(rect()).adjusted(2, 6, -2, -6);
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 t0 = now - plotter->window();
    auto X = [&](qint64 t) { return r.left() + (t - t0) * r.width() / plotter->window(); };
    QColor text = palette().color(QPalette::WindowText);
    QColor muted = text;
    muted.setAlphaF(0.55f);

    // the common scale: the lines in the window (not those with their own range) and the levels
    double mn = INFINITY, mx = -INFINITY;
    for(auto &l: def.lines) {
        if(l.own_range) {
            continue;
        }
        for(auto &pt: plotter->values(l.series)) {
            if(pt.t >= t0) {
                mn = qMin(mn, pt.v);
                mx = qMax(mx, pt.v);
            }
        }
    }
    bool any = std::isfinite(mn);
    for(auto &[v, name, color]: def.levels) {
        if(any) {     // only near the values: a level far below does not squash the plot
            double span = qMax(mx - mn, 1.0);
            if(v > mn - span && v < mx + span) {
                mn = qMin(mn, v);
                mx = qMax(mx, v);
            }
        }
    }
    if(!any) {
        p.setPen(muted);
        p.drawText(r, Qt::AlignCenter, "No data yet");
        return;
    }
    if(mx - mn < 1e-6) {
        mn -= 1;
        mx += 1;
    }
    double pad = (mx - mn) * 0.08;
    mn -= pad;
    mx += pad;
    auto Y = [&](double v, double lo, double hi) { return r.bottom() - (v - lo) / (hi - lo) * r.height(); };

    // the scale: its max and min in small text on the left
    QFont small = font();
    small.setPointSizeF(small.pointSizeF() * 0.8);
    p.setFont(small);
    p.setPen(muted);
    int dec = def.lines.isEmpty() ? 1 : def.lines.first().decimals;
    p.drawText(QRectF(r.left() + 2, r.top() - 4, 80, 14), Qt::AlignLeft | Qt::AlignTop, QString::number(mx - pad, 'f', dec));
    p.drawText(QRectF(r.left() + 2, r.bottom() - 10, 80, 14), Qt::AlignLeft | Qt::AlignBottom, QString::number(mn + pad, 'f', dec));

    // the levels: dashed lines with their name
    for(auto &[v, name, color]: def.levels) {
        if(v < mn || v > mx) {
            continue;
        }
        double y = Y(v, mn, mx);
        p.setPen(QPen(color, 1, Qt::DashLine));
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
        p.setPen(color);
        p.drawText(QRectF(r.right() - 80, y - 14, 80, 13), Qt::AlignRight | Qt::AlignBottom, name);
    }

    // the lines (from the last value before the window: no gap at its left edge)
    for(auto &l: def.lines) {
        double lo = l.own_range ? l.range_min : mn;
        double hi = l.own_range ? l.range_max : mx;
        auto &pts = plotter->values(l.series);
        QPainterPath path;
        bool started = false;
        for(int i = 0; i < pts.size(); i++) {
            if(pts[i].t < t0 && !(i + 1 < pts.size() && pts[i + 1].t >= t0)) {
                continue;
            }
            QPointF pt(qMax(r.left(), X(pts[i].t)), Y(pts[i].v, lo, hi));
            if(started) {
                path.lineTo(pt);
            } else {
                path.moveTo(pt);
                started = true;
            }
        }
        QPen pen(l.color, 1.8);
        if(l.dashed) {
            pen.setStyle(Qt::DashLine);
        }
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }

    // the moment under the mouse: a line and the values on it
    qint64 c = plotter->cursor();
    if(c >= t0 && c <= now) {
        double x = X(c);
        p.setPen(QPen(muted, 1));
        p.drawLine(QPointF(x, r.top() - 4), QPointF(x, r.bottom() + 4));
        for(auto &l: def.lines) {
            auto v = plotter->valueAt(l.series, c);
            if(v.has_value()) {
                double lo = l.own_range ? l.range_min : mn;
                double hi = l.own_range ? l.range_max : mx;
                p.setPen(Qt::NoPen);
                p.setBrush(l.color);
                p.drawEllipse(QPointF(x, Y(v.value(), lo, hi)), 3, 3);
            }
        }
    }
}

/////////////////////////////// Plotter ///////////////////////////////

Plotter::Plotter(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id)
{
    auto lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);

    // the aircraft and the moment shown, and the time window
    auto top = new QHBoxLayout();
    top->setContentsMargins(4, 0, 4, 0);
    status = new QLabel(this);
    status->setObjectName("card_muted");
    top->addWidget(status, 1);
    for(auto &[label, ms]: QList<QPair<QString, qint64>>{{"1 min", 60000}, {"5 min", 300000}, {"15 min", 900000}}) {
        auto b = new QPushButton(label, this);
        b->setObjectName("plot_window");
        b->setCheckable(true);
        b->setProperty("ms", ms);
        b->setCursor(Qt::PointingHandCursor);
        connect(b, &QPushButton::clicked, this, [=, this, ms=ms]() { setWindow(ms); save(); });
        top->addWidget(b);
        window_buttons.append(b);
    }
    // a time window of choice (1 to 60 min)
    custom_button = new QPushButton("Custom", this);
    custom_button->setObjectName("plot_window");
    custom_button->setCheckable(true);
    custom_button->setCursor(Qt::PointingHandCursor);
    custom_button->setToolTip("Another time window (1 to 60 min)");
    connect(custom_button, &QPushButton::clicked, this, [=, this]() {
        custom_button->setChecked(!window_is_preset());
        auto menu = new QMenu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        auto w = new QWidget(menu);
        auto h = new QHBoxLayout(w);
        h->setContentsMargins(10, 6, 10, 6);
        auto spin = new QSpinBox(w);
        spin->setRange(1, 60);
        spin->setSuffix(" min");
        spin->setValue(static_cast<int>(window_ms / 60000));
        auto ok = new QPushButton("Show", w);
        h->addWidget(spin);
        h->addWidget(ok);
        auto wa = new QWidgetAction(menu);
        wa->setDefaultWidget(w);
        menu->addAction(wa);
        auto apply = [=, this]() {
            setWindow(spin->value() * 60000LL);
            save();
            menu->close();
        };
        connect(ok, &QPushButton::clicked, menu, apply);
        connect(spin, &QSpinBox::editingFinished, menu, [=]() { if(spin->hasFocus()) apply(); });
        menu->popup(custom_button->mapToGlobal(QPoint(0, custom_button->height())));
        spin->setFocus();
        spin->selectAll();
    });
    top->addWidget(custom_button);
    lay->addLayout(top);

    // the plots, then + Add plot
    auto content = new QWidget(this);
    auto outer = new QVBoxLayout(content);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    rows_layout = new QVBoxLayout();
    rows_layout->setContentsMargins(0, 0, 0, 0);
    rows_layout->setSpacing(0);
    outer->addLayout(rows_layout, 1);
    auto add = new QPushButton("+ Add plot", content);
    add->setObjectName("plot_add");
    add->setCursor(Qt::PointingHandCursor);
    connect(add, &QPushButton::clicked, this, [=, this]() {
        auto menu = plotMenu([=, this](QString id) { addPlot(id); save(); });
        menu->popup(add->mapToGlobal(QPoint(0, add->height())));
    });
    auto add_row = new QHBoxLayout();
    add_row->setContentsMargins(4, 6, 4, 4);
    add_row->addWidget(add);
    add_row->addStretch(1);
    cursor_label = new QLabel(content);
    cursor_label->setObjectName("card_muted");
    cursor_label->setMinimumWidth(cursor_label->fontMetrics().horizontalAdvance("at 00:00:00") + 4);
    cursor_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    add_row->addWidget(cursor_label);
    outer->addLayout(add_row);
    lay->addWidget(content, 1);

    setStyleSheet(
        "QPushButton#plot_window {border: 1px solid #c4ccd5; border-radius: 10px; padding: 1px 10px; background: transparent;}"
        "QPushButton#plot_window:checked {background: #1d5fc9; border-color: #1d5fc9; color: white;}"
        "QPushButton#plot_add {border: 1px dashed #8c959f; border-radius: 6px; padding: 3px 12px; background: transparent;}"
        "QToolButton#plot_title {font-weight: bold; border: 1px solid #c4ccd5; border-radius: 6px; padding: 1px 6px;}"
        "QToolButton#plot_title::menu-indicator {image: none; width: 0;}"
        "QToolButton#plot_close {border: 0; background: transparent;}");

    seenTelemetry();     // (listens to the telemetry, for "Other message field")
    // the presets: recorded from the start
    PprzDispatcher::get()->bind("ENGINE_STATUS", this, [=, this](QString, pprzlink::Message msg) {
        QString id;
        msg.getField("ac_id", id);
        if(id != this->ac_id) {
            return;
        }
        qint64 t = QDateTime::currentMSecsSinceEpoch();
        float bat, amp, throttle, charge;
        msg.getField("bat", bat);
        msg.getField("amp", amp);
        msg.getField("throttle", throttle);
        msg.getField("charge", charge);
        record(BAT, bat, t);
        record(AMP, amp, t);
        record(THROTTLE, throttle, t);
        record(POWER, bat * amp, t);    // (the ENERGY message, when sent, replaces it)
        record(ENERGY, charge * 1000., t);
    });
    // telemetry, when the aircraft sends it: faster than the ground messages (1 Hz)
    auto telemetry = [=, this](QString name, std::function<void(pprzlink::Message&, qint64)> cb) {
        PprzDispatcher::get()->bindOptional(name, this, [=, this](QString sender, pprzlink::Message msg) {
            if(sender == this->ac_id) {
                try {
                    cb(msg, QDateTime::currentMSecsSinceEpoch());
                } catch(std::exception&) {
                }
            }
        });
    };
    telemetry("ENERGY", [=, this](pprzlink::Message& msg, qint64 t) {
        float voltage, current, power;
        msg.getField("voltage", voltage);
        msg.getField("current", current);
        msg.getField("power", power);
        record(BAT, voltage, t, 1);
        record(AMP, current, t, 1);
        record(POWER, power, t, 1);
    });
    telemetry("ATTITUDE", [=, this](pprzlink::Message& msg, qint64 t) {
        float phi, theta, psi;
        msg.getField("phi", phi);
        msg.getField("theta", theta);
        msg.getField("psi", psi);
        record(ROLL, qRadiansToDegrees(phi), t, 2);
        record(PITCH, qRadiansToDegrees(theta), t, 2);
        record(YAW, fmod(qRadiansToDegrees(psi) + 360., 360.), t, 2);
    });
    telemetry("ROTORCRAFT_FP", [=, this](pprzlink::Message& msg, qint64 t) {
        int32_t phi, theta, psi, veast, vnorth;
        msg.getField("phi", phi);
        msg.getField("theta", theta);
        msg.getField("psi", psi);
        msg.getField("veast", veast);
        msg.getField("vnorth", vnorth);
        const double att = 0.0139882, spd = 0.0000019;
        record(ROLL, phi * att, t, 1);
        record(PITCH, theta * att, t, 1);
        record(YAW, fmod(psi * att + 360., 360.), t, 1);
        record(SPEED, std::hypot(veast * spd, vnorth * spd), t, 1);
    });
    telemetry("AIRSPEED", [=, this](pprzlink::Message& msg, qint64 t) {
        float airspeed;
        msg.getField("airspeed", airspeed);
        record(AIRSPEED, airspeed, t, 1);
    });
    PprzDispatcher::get()->bind("FLIGHT_PARAM", this, [=, this](QString, pprzlink::Message msg) {
        QString id;
        msg.getField("ac_id", id);
        if(id != this->ac_id) {
            return;
        }
        qint64 t = QDateTime::currentMSecsSinceEpoch();
        float speed, airspeed, agl, alt, roll, pitch, heading;
        msg.getField("roll", roll);
        msg.getField("pitch", pitch);
        msg.getField("heading", heading);
        record(ROLL, roll, t);
        record(PITCH, pitch, t);
        record(YAW, fmod(heading + 360., 360.), t);
        msg.getField("speed", speed);
        msg.getField("airspeed", airspeed);
        msg.getField("agl", agl);
        msg.getField("alt", alt);
        record(SPEED, speed, t);
        record(AIRSPEED, airspeed, t);
        record(AGL, agl, t);
        last_alt = alt;
        last_agl = agl;
        has_alt = true;
    });
    PprzDispatcher::get()->bind("NAV_STATUS", this, [=, this](QString, pprzlink::Message msg) {
        QString id;
        msg.getField("ac_id", id);
        if(id != this->ac_id || !has_alt) {
            return;
        }
        float target_alt;
        msg.getField("target_alt", target_alt);
        record(TARGET_AGL, target_alt - (last_alt - last_agl), QDateTime::currentMSecsSinceEpoch());
    });

    // the plots move along with the time
    refresh = new QTimer(this);
    refresh->setInterval(200);     // smooth enough, for the telemetry rates
    connect(refresh, &QTimer::timeout, this, [=, this]() {
        if(isVisible()) {
            for(auto &r: rows) {
                r.chart->update();
            }
            updateValues();
        }
    });
    refresh->start();

    setAcceptDrops(true);
    // the plots of the layout (configure), or the default one
    QTimer::singleShot(0, this, [=, this]() {
        if(!configured) {
            configure(QDomElement());
        }
    });
}

/// A value from a source of a level: the ground messages 0, telemetry higher (faster). Once a
/// higher level was received for a series, the lower ones are not used anymore.
void Plotter::record(QString series, double v, qint64 t, int level) {
    if(level < best_level.value(series, 0)) {
        return;
    }
    best_level[series] = level;
    auto &d = data[series];
    d.append({t, v});
    while(!d.isEmpty() && d.first().t < t - KEEP_MS) {
        d.removeFirst();
    }
}

std::optional<double> Plotter::valueAt(const QString& series, qint64 t) {
    auto &d = data[series];
    std::optional<double> v;
    for(auto &pt: d) {
        if(pt.t > t) {
            break;
        }
        v = pt.v;
    }
    return v;
}

PlotDef Plotter::preset(QString id) {
    PlotDef d;
    d.id = id;
    if(id == "battery") {
        d.title = "Battery";
        d.unit = "V";
        d.lines = {{BAT, "", BLUE, false, 2}};
        // the low and critical levels of the airframe
        auto airframe = AircraftManager::get()->getAircraft(ac_id)->getAirframe();
        auto low = airframe->getDefine("LOW_BAT_LEVEL");
        auto critic = airframe->getDefine("CRITIC_BAT_LEVEL");
        d.levels.append({low.has_value() ? low->value.toDouble() : 10.5, "low", ORANGE});
        d.levels.append({critic.has_value() ? critic->value.toDouble() : 10., "critical", RED});
    } else if(id == "current") {
        d.title = "Current · throttle";
        d.lines = {{AMP, "A", ORANGE, false, 1},
                   {THROTTLE, "thr %", GREY, true, 0, true, 0, 100}};
    } else if(id == "power") {
        d.title = "Power";
        d.unit = "W";
        d.lines = {{POWER, "", MAGENTA, false, 0}};
    } else if(id == "energy") {
        d.title = "Energy used";
        d.unit = "mAh";
        d.lines = {{ENERGY, "", PURPLE, false, 0}};
    } else if(id == "speed") {
        d.title = "Speed";
        d.unit = "m/s";
        d.lines = {{SPEED, "ground", BLUE, false, 1}, {AIRSPEED, "air", GREEN, false, 1}};
    } else if(id == "attitude") {
        d.title = "Attitude";
        d.unit = QString::fromUtf8("\u00b0");
        d.lines = {{ROLL, "roll", BLUE, false, 0}, {PITCH, "pitch", GREEN, false, 0},
                   {YAW, "yaw", GREY, true, 0, true, 0, 360}};
    } else if(id == "height") {
        d.title = "Height";
        d.unit = "m";
        d.lines = {{AGL, "AGL", BLUE, false, 0}, {TARGET_AGL, "target", GREY, true, 0}};
    } else {
        return fieldPlot(id);
    }
    return d;
}

/// A plot of a message field: "class:MESSAGE:field" or "class:MESSAGE:field[index]"
PlotDef Plotter::fieldPlot(QString series, int index) {
    PlotDef d;
    d.id = series;
    auto parts = series.split(":");
    d.title = parts.size() >= 3 ? parts[1] + " " + parts[2] : series;
    d.lines = {{series, "", BLUE, false, 2}};
    QRegularExpression re("\\[(\\d+)\\]$");
    auto m = re.match(series);
    listen(series, index >= 0 ? index : (m.hasMatch() ? m.captured(1).toInt() : -1));
    return d;
}

/// Record a message field (not a preset), from now on
void Plotter::listen(QString series, int index) {
    if(listened.contains(series)) {
        return;
    }
    QString key = series;
    key.remove(QRegularExpression("\\[\\d+\\]$"));
    auto parts = key.split(":");
    if(parts.size() < 3) {
        return;
    }
    listened.insert(series);
    QString msg_class = parts[0], field = parts[2];
    PprzDispatcher::get()->bind(parts[1], this, [=, this](QString sender, pprzlink::Message msg) {
        if(msg_class == "telemetry") {
            if(sender != ac_id) {
                return;
            }
        } else {
            QString id;
            try {
                msg.getField("ac_id", id);
            } catch(std::exception&) {
                return;
            }
            if(id != ac_id) {
                return;
            }
        }
        auto v = fieldValue(msg, field, index);
        if(v.has_value()) {
            record(series, v.value() * scales.value(series, 1.0), QDateTime::currentMSecsSinceEpoch());
        }
    });
}

/// The plots of the layout: <plot preset="battery"/>, or a field as before:
/// <plot name="ground:ENGINE_STATUS:bat:1."/> (class:message:field:scale). The plots chosen by
/// the user (saved per layout) replace them.
void Plotter::configure(QDomElement c) {
    configured = true;
    QStringList ids;
    for(auto node = c.firstChildElement("plot"); !node.isNull(); node = node.nextSiblingElement("plot")) {
        if(node.hasAttribute("preset")) {
            ids.append(node.attribute("preset"));
        } else if(node.hasAttribute("name")) {
            auto def = node.attribute("name").split(":");
            if(def.size() == 4) {
                QString series = def[0] + ":" + def[1] + ":" + def[2];
                // the old battery plot is the battery preset
                if(series == BAT) {
                    ids.append("battery");
                    continue;
                }
                bool ok;
                double scale = def[3].toDouble(&ok);
                scales[series] = ok ? scale : 1.0;
                ids.append(series);
            }
        }
    }
    auto settings = getAppSettings();
    QString key = "plots/" + QFileInfo(settings.value("APP_LAYOUT_FILE").toString()).baseName();
    if(settings.contains(key + "/plots")) {
        ids = settings.value(key + "/plots").toStringList();
    }
    if(ids.isEmpty()) {
        ids = {"battery"};
    }
    setWindow(settings.value(key + "/window", window_ms).toLongLong());
    plot_ids.clear();
    for(auto &id: ids) {
        if(presetIds().contains(id) || id.count(":") >= 2) {
            plot_ids.append(id);
        }
    }
    rebuildRows();
}

void Plotter::save() {
    auto settings = getAppSettings();
    QString key = "plots/" + QFileInfo(settings.value("APP_LAYOUT_FILE").toString()).baseName();
    settings.setValue(key + "/plots", plot_ids);
    settings.setValue(key + "/window", window_ms);
}

void Plotter::setWindow(qint64 ms) {
    window_ms = ms;
    for(auto b: window_buttons) {
        b->setChecked(b->property("ms").toLongLong() == ms);
    }
    if(custom_button != nullptr) {
        custom_button->setChecked(!window_is_preset());
        custom_button->setText(window_is_preset() ? QString("Custom") : QString("%1 min").arg(ms / 60000));
    }
    for(auto &r: rows) {
        r.chart->update();
    }
}

void Plotter::addPlot(QString id, int at) {
    if(at < 0) {
        plot_ids.append(id);
    } else {
        plot_ids.insert(at, id);
    }
    rebuildRows();
}

void Plotter::setPlot(int row, QString id) {
    if(row >= 0 && row < plot_ids.size()) {
        plot_ids[row] = id;
        rebuildRows();
        save();
    }
}

void Plotter::removePlot(int row) {
    if(plot_ids.size() > 1 && row >= 0 && row < plot_ids.size()) {
        plot_ids.removeAt(row);
        rebuildRows();
        save();
    }
}

/// The plots to choose from: the presets, and any field of a ground message
QMenu* Plotter::plotMenu(std::function<void(QString)> chosen) {
    auto menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    for(auto &id: presetIds()) {
        auto a = menu->addAction(preset(id).title);
        connect(a, &QAction::triggered, this, [=]() { chosen(id); });
    }
    menu->addSeparator();
    // the fields of the telemetry messages received from this aircraft
    auto other = menu->addMenu("Other message field");
    auto names = seenTelemetry().value(ac_id).values();
    std::sort(names.begin(), names.end());
    if(names.isEmpty()) {
        other->addAction("No telemetry received yet")->setEnabled(false);
    }
    auto msg_dict = PprzDispatcher::get()->getDict();
    for(auto &name: names) {
        std::optional<pprzlink::MessageDefinition> def;
        try {
            def = msg_dict->getDefinition(name);
        } catch(std::exception&) {
            continue;
        }
        QMenu* msg_menu = nullptr;
        for(size_t i = 0; i < def->getNbFields(); ++i) {
            auto f = def->getField(static_cast<int>(i));
            auto t = f.getType();
            if(t.getBaseType() == pprzlink::BaseType::STRING || t.getBaseType() == pprzlink::BaseType::CHAR) {
                continue;
            }
            if(msg_menu == nullptr) {
                msg_menu = other->addMenu(name);
            }
            QString series = "telemetry:" + name + ":" + f.getName();
            if(t.isArray()) {
                // the elements of a fixed array (up to 16)
                size_t n = t.getArraySize();
                for(size_t k = 0; k < n && k < 16; k++) {
                    QString se = series + QString("[%1]").arg(k);
                    auto a = msg_menu->addAction(QString("%1[%2]").arg(f.getName()).arg(k));
                    connect(a, &QAction::triggered, this, [=]() { chosen(se); });
                }
                continue;
            }
            auto a = msg_menu->addAction(f.getName());
            connect(a, &QAction::triggered, this, [=]() { chosen(series); });
        }
    }
    return menu;
}

void Plotter::rebuildRows() {
    for(auto &r: rows) {
        rows_layout->removeWidget(r.widget);
        r.widget->deleteLater();
    }
    rows.clear();
    for(int i = 0; i < plot_ids.size(); i++) {
        auto def = preset(plot_ids[i]);
        auto w = new QWidget(this);
        auto v = new QVBoxLayout(w);
        v->setContentsMargins(4, 6, 4, 2);
        v->setSpacing(2);
        auto head = new QHBoxLayout();
        auto title = new QToolButton(w);
        title->setObjectName("plot_title");
        title->setText(def.title + QString::fromUtf8(" ▾"));
        title->setToolTip("Change this plot");
        title->setCursor(Qt::PointingHandCursor);
        connect(title, &QToolButton::clicked, this, [=, this]() {
            auto menu = plotMenu([=, this](QString id) { setPlot(i, id); });
            menu->popup(title->mapToGlobal(QPoint(0, title->height())));
        });
        head->addWidget(title);
        head->addStretch(1);
        auto value = new QLabel(w);
        value->setTextFormat(Qt::RichText);
        value->setFixedHeight(QFontMetrics(value->font()).height() + 8);     // the same with or without a value
        head->addWidget(value);
        if(plot_ids.size() > 1) {
            auto close = new QToolButton(w);
            close->setObjectName("plot_close");
            close->setText(QString::fromUtf8("✕"));
            close->setToolTip("Remove this plot");
            connect(close, &QToolButton::clicked, this, [=, this]() { removePlot(i); });
            head->addWidget(close);
        }
        v->addLayout(head);
        auto chart = new PlotChart(this, def, w);
        connect(chart, &PlotChart::hovered, this, [=, this](qint64 t) {
            cursor_time = t;
            for(auto &r: rows) {
                r.chart->update();
            }
            updateValues();
        });
        v->addWidget(chart, 1);
        if(i > 0) {
            w->setObjectName("plot_row");
            w->setStyleSheet("#plot_row {border-top: 1px solid rgba(128, 128, 128, 60);}");
        }
        rows_layout->addWidget(w, 1);
        rows.append({w, title, value, chart, def});
    }
    updateValues();
}

/// The values shown: at the moment under the mouse, or the last ones
void Plotter::updateValues() {
    QString name = AircraftManager::get()->aircraftExists(ac_id) ? AircraftManager::get()->getAircraft(ac_id)->name() : ac_id;
    status->setText(name);
    // the moment under the mouse: below the plots, in its own place (nothing moves)
    cursor_label->setText(cursor_time >= 0 ? "at " + QDateTime::fromMSecsSinceEpoch(cursor_time).toString("HH:mm:ss") : QString());
    for(auto &r: rows) {
        QStringList parts;
        for(auto &l: r.def.lines) {
            std::optional<double> v;
            if(cursor_time >= 0) {
                v = valueAt(l.series, cursor_time);
            } else if(!data[l.series].isEmpty()) {
                v = data[l.series].last().v;
            }
            QString num = v.has_value() ? QString::number(v.value(), 'f', l.decimals) : QString::fromUtf8("–");
            QString part;
            if(!l.name.isEmpty()) {
                part = QString("<span style='color:#57606a; font-size:11px;'>%1</span> ").arg(l.name.toHtmlEscaped());
            }
            part += QString("<span style='color:%1; font-family:Monospace; font-size:14px; font-weight:600;'>%2</span>")
                        .arg(l.color.name(), num);
            parts.append(part);
        }
        QString unit = r.def.unit.isEmpty() ? QString() :
            QString(" <span style='color:#57606a; font-size:12px;'>%1</span>").arg(r.def.unit.toHtmlEscaped());
        r.value->setText(parts.join("&nbsp;&nbsp;") + unit);
    }
}

void Plotter::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat("text/plain")) {
        event->acceptProposedAction();
    }
    event->accept();
}

void Plotter::dragMoveEvent(QDragMoveEvent *event) {
    event->accept();
}

/**
 * A field dropped from the messages tool: "<id>:<class>:<msg name>:<field>[index]:<scale>"
 * (ex: "2:telemetry:WP_MOVED:utm_north:1."): a plot of it, below the others.
 */
void Plotter::dropEvent(QDropEvent *event) {
    QString text = QString::fromUtf8(event->mimeData()->data("text/plain"));
    QRegularExpression pprz_msg_re("^(\\w+):(\\w+):(\\w+):(\\w+)(?:\\[(.*)\\])?:(.*)$");
    auto m = pprz_msg_re.match(text);
    if(!m.hasMatch() || m.captured(1) != ac_id) {
        return;
    }
    QString series = QString("%1:%2:%3").arg(m.captured(2), m.captured(3), m.captured(4));
    int index = -1;
    QRegularExpression array_re("^(\\d+)(?:-(\\d+))?$");
    auto am = array_re.match(m.captured(5));
    if(am.hasMatch()) {
        index = am.captured(1).toInt();
        series += QString("[%1]").arg(index);
    }
    bool ok;
    double scale = m.captured(6).toDouble(&ok);
    scales[series] = ok ? scale : 1.0;
    listen(series, index);
    addPlot(series);
    save();
}
