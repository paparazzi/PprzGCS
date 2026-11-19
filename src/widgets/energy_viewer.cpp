#include "energy_viewer.h"
#include "AircraftManager.h"
#include "gcs_utils.h"
#include "airframe.h"
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSpinBox>
#include <QToolButton>
#include <QWhatsThis>
#include <QDoubleSpinBox>
#include <cmath>
#include <algorithm>
#include <vector>

namespace {

const int MIN_WIDTH = 260;
const double CHIP_H = 20;
const double CHIPS = CHIP_H + 6;        // the pills of the estimator
const double HEADER = CHIPS + 76;
const double CURVE_HEIGHT = 130;
const double FOOTER = 36;               // charge, time and resistance under the plot
const double HISTORY_H = 96;            // V - I.R: the history of the voltage and the load
const double HISTORY_S = 300;           // s shown
const double CURVE_MIN = 90;            // the curve, made smaller for the history
const double TRACE_DT = 1;              // s between the points of the voltage over the flight
const size_t TRACE_MAX = 7200;          // then every other point is dropped
const double TRACE_NEW = 0.1;           // the energy went up more: another battery, the trace starts again

const QColor GOOD(0x1f7a3f);
const QColor ACCENT(0xf0a020);          // as in the ATC panel
const QColor LOW = ACCENT;
const QColor EMPTY(0xd92d20);

const char *METHOD_NAMES[EnergyEstimator::METHODS] = {"V", "mAh", "V \xE2\x88\x92 I\xC2\xB7R", "Filter"};

QColor withAlpha(QColor c, int alpha) {
    c.setAlpha(alpha);
    return c;
}

QFont pixelFont(const QFont &base, int px, bool bold) {
    QFont f = base;
    f.setPixelSize(px);
    f.setBold(bold);
    return f;
}

QString chemistryName(EnergyEstimator::Chemistry c) {
    switch(c) {
    case EnergyEstimator::LIHV: return "LiHV";
    case EnergyEstimator::LIION: return "Li-ion";
    case EnergyEstimator::LIFE: return "LiFe";
    default: return "LiPo";
    }
}

/// BAT_CHEMISTRY of an airframe (LIPO, LIHV, LIION, LIFE, a few spellings), -1: not known
int chemistryOf(QString name) {
    QString n = name.toUpper().remove('_').remove('-').remove(' ').remove('"');
    if(n == "LIPO") return EnergyEstimator::LIPO;
    if(n == "LIHV" || n == "LIPOHV") return EnergyEstimator::LIHV;
    if(n == "LIION") return EnergyEstimator::LIION;
    if(n == "LIFE" || n == "LIFEPO4") return EnergyEstimator::LIFE;
    return -1;
}

/// the range of the plot: the curve, and room under it for the sag
void plotRange(EnergyEstimator::Chemistry c, double &vmin, double &vmax) {
    vmin = std::floor((EnergyEstimator::ocvOfSoc(c, 0.05) - 0.4) * 10) / 10;
    vmax = std::ceil((EnergyEstimator::ocvOfSoc(c, 1) + 0.05) * 10) / 10;
}

}   // namespace


EnergyViewer::EnergyViewer(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id)
{
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    ac_name = ac->name();
    if(ac->getAirframe() != nullptr) {
        auto cells = ac->getAirframe()->getDefine("BAT_NB_CELLS");
        if(cells && cells->value.toInt() > 0) {
            airframe_cells = cells->value.toInt();
            estimator.setCells(airframe_cells);
            cells_known = true;
        }
        auto cap = ac->getAirframe()->getDefine("BAT_CAPACITY");
        if(cap && cap->value.toDouble() > 0) {
            // in mAh; also accepted in Ah
            airframe_capacity = cap->value.toDouble() * (cap->unit.compare("Ah", Qt::CaseInsensitive) == 0 ? 1000 : 1);
            estimator.setCapacity(airframe_capacity);
        }
        // the alarm levels of the aircraft, drawn on the curve: the pack voltages the alerts use
        // (not set: the default per cell times BAT_NB_CELLS, else 3 cells); marked when a default
        level_low = ac->getAirframe()->batteryLevel("LOW_BAT_LEVEL");
        level_critic = ac->getAirframe()->batteryLevel("CRITIC_BAT_LEVEL");
        level_catastrophic = ac->getAirframe()->batteryLevel("CATASTROPHIC_BAT_LEVEL");
        for(auto name : {"LOW_BAT_LEVEL", "CRITIC_BAT_LEVEL", "CATASTROPHIC_BAT_LEVEL"}) {
            auto d = ac->getAirframe()->getDefine(name);
            if(!(d && d->value.toDouble() > 0)) {
                level_defaults << name;
            }
        }
        auto chem = ac->getAirframe()->getDefine("BAT_CHEMISTRY");
        if(chem) {
            int c = chemistryOf(chem->value);
            if(c >= 0) {
                airframe_chemistry = static_cast<EnergyEstimator::Chemistry>(c);
                airframe_chemistry_set = true;
            } else {
                qWarning() << "Energy:" << ac_name << ": BAT_CHEMISTRY" << chem->value
                           << "not known (LIPO, LIHV, LIION or LIFE), LiPo used";
            }
        }
        auto res = ac->getAirframe()->getDefine("BAT_CELL_RESISTANCE");
        if(res && res->value.toDouble() > 0) {
            // per cell, in mOhm (also accepted in Ohm)
            airframe_resistance = res->value.toDouble() * (res->unit.compare("Ohm", Qt::CaseInsensitive) == 0 ? 1000 : 1);
        }
    }
    // the choices of the user, per aircraft, over the airframe (the chemistry, cells, capacity and R
    // are saved only when they differ from the airframe: else they follow it, also when it changes)
    auto settings = getAppSettings();
    QString key = "energy/" + ac_name + "/";
    estimator.setChemistry(static_cast<EnergyEstimator::Chemistry>(
        qBound(0, settings.value(key + "chemistry", airframe_chemistry).toInt(), EnergyEstimator::CHEMISTRIES - 1)));
    estimator.setStartResistance(settings.value(key + "resistance", airframe_resistance).toDouble() / 1000);
    estimator.setMethod(static_cast<EnergyEstimator::Method>(
        qBound(0, settings.value(key + "estimator", EnergyEstimator::VOLTAGE).toInt(), EnergyEstimator::METHODS - 1)));
    if(settings.contains(key + "cells")) {
        estimator.setCells(settings.value(key + "cells").toInt());     // 0: guessed from the voltage
        cells_known = estimator.getCells() > 0;
    }
    if(settings.contains(key + "capacity")) {
        estimator.setCapacity(settings.value(key + "capacity").toDouble());
    }
    estimator.setReserve(settings.value(key + "reserve", 0.2).toDouble());
    estimator.setSag(settings.value(key + "sag", 0.0).toDouble());
    clock.start();

    connect(ac->getStatus(), &AircraftStatus::engine_status, this, &EnergyViewer::addEngineStatus);
    // the help: on a "?" (as in the app settings), not as a tooltip over the whole panel
    help_button = new QToolButton(this);
    help_button->setText("?");
    help_button->setAutoRaise(true);
    help_button->setCursor(Qt::WhatsThisCursor);
    help_button->setToolTip("Help");
    help_button->setStyleSheet("QToolButton { font-weight: bold; color: palette(placeholder-text); "
                               "border: 1px solid palette(mid); border-radius: 9px; min-width: 16px; max-width: 16px; "
                               "min-height: 16px; max-height: 16px; padding: 0px; }");
    connect(help_button, &QToolButton::clicked, this, [this]() {
        QWhatsThis::showText(help_button->mapToGlobal(QPoint(help_button->width() / 2, help_button->height())),
            "Energy left (Wh) from the voltage, corrected for the sag under load "
            "(the current when measured, else the throttle).<br>"
            "<b>V</b>: the voltage + \xCE\x94V set in Battery\xE2\x80\xA6 (while the motors run); "
            "with \xCE\x94V 0, the voltage alone: low under load, never too high.<br>"
            "<b>mAh</b>: the charge used (sent by the aircraft, else the current counted) over the capacity.<br>"
            "<b>V \xE2\x88\x92 I\xC2\xB7R</b>: the voltage + 60 % of R \xC3\x97 load, R learned from the changes of the load.<br>"
            "<b>Filter</b>: the mAh count, pulled slowly towards V \xE2\x88\x92 I\xC2\xB7R.<br>"
            "Click the battery line or right click: chemistry, cells, capacity, reserve.", help_button);
    });
}

void EnergyViewer::addEngineStatus()
{
    auto msg = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getMessage("ENGINE_STATUS");
    if(!msg) {
        return;
    }
    EnergyEstimator::Sample s;
    s.t = clock.elapsed() / 1000.;
    s.voltage = getFloatingField(*msg, "bat");
    double amp = getFloatingField(*msg, "amp");
    s.throttle = getFloatingField(*msg, "throttle");
    s.charge = getFloatingField(*msg, "charge");
    if(amp > 0.3) {
        seen_current = true;
    }
    s.current = seen_current ? std::max(0.0, amp) : -1;    // some aircraft send -1e-6 for 0
    if(s.voltage <= 0) {
        return;
    }
    estimator.addSample(s);
    result = estimator.estimate();
    if(result.valid) {
        if(!history.empty() && s.t < history.back().t) {
            history.clear();
        }
        history.push_back({s.t, result.cell, result.load, result.load_x});
        while(history.front().t < s.t - HISTORY_S) {
            history.pop_front();
        }
        // the voltage over the whole flight, against the energy used by each method
        if(trace.empty() || s.t >= trace.back().t + TRACE_DT || s.t < trace.back().t) {
            Trace tr;
            tr.t = s.t;
            tr.cell = result.cell;
            for(int i = 0; i < EnergyEstimator::METHODS; i++) {
                auto e = estimator.estimate(static_cast<EnergyEstimator::Method>(i));
                // a method that cannot run (yet) falls back on another: not its own estimate
                tr.energy[i] = e.valid && e.method == i ? e.energy : -1;
            }
            int m = result.method;
            bool restart = s.t < (trace.empty() ? 0 : trace.back().t);
            for(auto it = trace.rbegin(); !restart && it != trace.rend(); ++it) {
                if(it->energy[m] >= 0) {
                    restart = tr.energy[m] > it->energy[m] + TRACE_NEW;
                    break;
                }
            }
            if(restart) {
                trace.clear();
            }
            trace.push_back(tr);
            if(trace.size() > TRACE_MAX) {
                std::vector<Trace> half;
                for(size_t i = 0; i < trace.size(); i += 2) {
                    half.push_back(trace[i]);
                }
                trace.swap(half);
            }
        }
    }
    update();
}

void EnergyViewer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    help_button->move(width() - help_button->width(), (static_cast<int>(CHIP_H) - help_button->height()) / 2);
}

QSize EnergyViewer::sizeHint() const
{
    // room for the history of V - I.R under a full curve
    return QSize(300, minimumSizeHint().height() + static_cast<int>(CURVE_MIN + HISTORY_H - CURVE_HEIGHT));
}

QSize EnergyViewer::minimumSizeHint() const
{
    return QSize(MIN_WIDTH, static_cast<int>(HEADER + CURVE_HEIGHT + FOOTER + 4));
}

double EnergyViewer::drawChips(QPainter &p)
{
    const QColor text = palette().color(QPalette::WindowText);
    const QColor caption = withAlpha(text, 150);
    const QColor line = withAlpha(text, 50);
    QFont f = pixelFont(font(), 11, false);
    p.setFont(f);
    QFontMetricsF fm(f);
    auto chip = [&](double x, double y, QString label, bool on, bool enabled = true) {
        QRectF r(x, y, fm.horizontalAdvance(label) + 14, CHIP_H);
        p.setPen(Qt::NoPen);
        p.setBrush(on ? ACCENT : line);
        p.drawRoundedRect(r, 5, 5);
        p.setPen(on ? QColor(Qt::black) : enabled ? caption : withAlpha(text, 80));
        p.drawText(r, Qt::AlignCenter, label);
        return r;
    };

    // the estimator: pills, changed often in flight
    double x = 0;
    for(int i = 0; i < EnergyEstimator::METHODS; i++) {
        bool on = estimator.getMethod() == i;
        bool enabled = !result.valid || result.ready[i];
        // V with a sag set: say so, it is no longer the voltage alone
        QString name = i == EnergyEstimator::VOLTAGE && estimator.getSag() > 0 ? QString::fromUtf8("V + \xCE\x94V") : METHOD_NAMES[i];
        method_chips[i] = chip(x, 0, name, on, enabled);
        x = method_chips[i].right() + 4;
    }
    p.setBrush(Qt::NoBrush);
    return CHIPS;
}

void EnergyViewer::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QColor text = palette().color(QPalette::WindowText);
    const QColor secondary = withAlpha(text, 170);
    const QString none = QString::fromUtf8("\xE2\x80\x94");    // em dash
    const auto &r = result;
    const double reserve = estimator.getReserve();
    QColor level = !r.valid ? secondary : r.energy <= reserve ? EMPTY : r.energy <= reserve + 0.2 ? LOW : GOOD;

    double y = drawChips(p);

    // energy left
    p.setFont(pixelFont(font(), 11, false));
    p.setPen(secondary);
    p.drawText(QRectF(0, y, width(), 16), Qt::AlignLeft | Qt::AlignVCenter, "Energy left (Wh)");
    if(r.valid && r.wh_left >= 0) {
        p.drawText(QRectF(0, y, width(), 16), Qt::AlignRight | Qt::AlignVCenter,
                   QString("%1 Wh").arg(r.wh_left, 0, 'f', r.wh_left < 10 ? 1 : 0));
    }
    p.setFont(pixelFont(font(), 22, true));
    p.setPen(r.valid ? text : secondary);
    p.drawText(QRectF(0, y + 14, width(), 28), Qt::AlignLeft | Qt::AlignVCenter,
               r.valid ? QString("%1 %").arg(qRound(r.energy * 100)) : none);

    // the bar, with the reserve marked
    QRectF bar(0, y + 44, width(), 10);
    p.setPen(Qt::NoPen);
    p.setBrush(withAlpha(text, 30));
    p.drawRoundedRect(bar, 5, 5);
    if(r.valid) {
        p.setBrush(level);
        p.drawRoundedRect(QRectF(bar.left(), bar.top(), bar.width() * qBound(0.0, r.energy, 1.0), bar.height()), 5, 5);
    }
    p.setPen(QPen(text, 1.5));
    double rx = bar.left() + bar.width() * reserve;
    p.drawLine(QPointF(rx, bar.top() - 2), QPointF(rx, bar.bottom() + 2));

    // the battery (click: its settings), and what the estimate still needs
    p.setFont(pixelFont(font(), 11, false));
    p.setPen(secondary);
    QString pack = QString("%1S %2, %3").arg(estimator.getCells() > 0 ? QString::number(estimator.getCells()) : "?")
                   .arg(chemistryName(estimator.getChemistry()))
                   .arg(estimator.getCapacity() > 0 ? QString("%1 mAh").arg(qRound(estimator.getCapacity())) : "capacity ?");
    if(r.valid) {
        pack += QString(", %1 V/cell (rest %2)").arg(r.cell, 0, 'f', 2).arg(r.ocv, 0, 'f', 2);
        if(r.power > 0) {
            pack += QString(", %1 W").arg(qRound(r.power));
        }
    } else {
        pack += ", waiting for ENGINE_STATUS";
    }
    QFontMetricsF fm(p.font());
    QString link = QString::fromUtf8("Battery\xE2\x80\xA6");
    const double gap = 10;
    battery_line = QRectF(0, y + 58, width(), 14);
    QString shown = fm.elidedText(pack, Qt::ElideRight, width() - fm.horizontalAdvance(link) - gap);
    p.drawText(battery_line, Qt::AlignLeft | Qt::AlignVCenter, shown);
    QFont lf = p.font();
    lf.setUnderline(true);
    p.setFont(lf);
    p.setPen(text);
    p.drawText(QRectF(fm.horizontalAdvance(shown) + gap, y + 58, width(), 14), Qt::AlignLeft | Qt::AlignVCenter, link);
    p.setFont(pixelFont(font(), 11, false));

    QString hint;
    const auto m = estimator.getMethod();
    if(r.valid && (m == EnergyEstimator::CHARGE || m == EnergyEstimator::FILTER) && !r.ready[m]) {
        hint = estimator.getCapacity() <= 0 ? "Set the capacity (Battery\xE2\x80\xA6) to count the mAh"
                                            : "No current nor mAh sent: nothing to count";
        hint += m == EnergyEstimator::FILTER ? "; V \xE2\x88\x92 I\xC2\xB7R meanwhile" : "; V meanwhile";
    } else if(r.valid && (r.method == EnergyEstimator::SAG || r.method == EnergyEstimator::FILTER) && !r.r_known) {
        hint = r.with_current ? "R not known yet: change the current" : "R not known yet: change the throttle";
    }
    double curve_top = y + 74;
    if(!hint.isEmpty()) {
        // wrapped: a narrow panel (a card on the map) shows all of it
        QRectF hr = fm.boundingRect(QRectF(0, y + 73, width(), 100), Qt::AlignLeft | Qt::TextWordWrap, hint);
        p.setPen(LOW);
        p.drawText(QRectF(0, y + 73, width(), hr.height()), Qt::AlignLeft | Qt::TextWordWrap, hint);
        curve_top += hr.height();
    }

    // the curve gives the room taken by the hint, so that the values under it stay visible
    double curve_h = qMax(CURVE_HEIGHT - (curve_top - y - 74), height() - curve_top - FOOTER - 2);
    // under the curve, in every estimator: the voltage and the load (from which R is learned)
    // (only when there is room: a small panel keeps the curve)
    double history_h = qMin(qMax(HISTORY_H, 0.4 * curve_h), curve_h - CURVE_MIN);
    bool show_history = history_h >= 50;
    if(show_history) {
        curve_h -= history_h;
    }
    drawCurve(p, QRectF(0, curve_top, width(), curve_h));
    if(show_history) {
        drawHistory(p, QRectF(0, curve_top + curve_h, width(), history_h));
        curve_h += history_h;
    }

    // under the plot: the charge left, the flight time to the reserve, and what the estimator uses
    QRectF foot(0, curve_top + curve_h + 2, width(), FOOTER);
    double cw = foot.width() / 3;
    auto stat = [&](int i, QString caption, QString value, QColor color = QColor()) {
        QRectF c(foot.left() + i * cw, foot.top(), cw, foot.height());
        p.setFont(pixelFont(font(), 10, false));
        p.setPen(secondary);
        p.drawText(QRectF(c.left(), c.top(), c.width(), 13), Qt::AlignLeft | Qt::AlignVCenter, caption);
        p.setFont(pixelFont(font(), 15, true));
        p.setPen(!r.valid ? secondary : color.isValid() ? color : text);
        p.drawText(QRectF(c.left(), c.top() + 13, c.width(), 20), Qt::AlignLeft | Qt::AlignVCenter, value);
    };
    stat(0, "Charge left", r.valid ? QString("%1 %").arg(qRound(r.soc * 100)) : none);
    // below the reserve: the time to empty, in red
    stat(1, r.valid && r.to_empty ? QString("Time to 0 %") : QString("Time to %1 %").arg(qRound(reserve * 100)),
         r.minutes >= 0 ? QString::fromUtf8("\xE2\x89\x88 %1 min").arg(r.minutes, 0, 'f', r.minutes < 10 ? 1 : 0) : none,
         r.valid && r.to_empty ? EMPTY : QColor());
    switch(r.method) {
    case EnergyEstimator::VOLTAGE:
        stat(2, QString::fromUtf8("\xCE\x94V/cell (set)"), QString("%1 mV").arg(qRound(estimator.getSag() * 1000)));
        break;
    case EnergyEstimator::CHARGE:
        stat(2, "Used", r.used >= 0 ? QString("%1 mAh").arg(qRound(r.used * 1000)) : none);
        break;
    default:
        if(!r.valid || !r.r_known) {
            stat(2, "Resistance/cell", none);
        } else if(r.with_current) {
            stat(2, "Resistance/cell", QString::fromUtf8("%1 m\xCE\xA9").arg(r.resistance * 1000, 0, 'f', 1));
        } else {
            stat(2, "Sag/cell per throttle", QString("%1 mV/%").arg(r.resistance * 1000, 0, 'f', 2));
        }
    }
}

/// The last minutes: the measured cell voltage (top) and load (bottom) in thin gray, and the
/// load low-passed, which V - I.R uses for the polarization, in orange
void EnergyViewer::drawHistory(QPainter &p, QRectF area)
{
    const QColor text = palette().color(QPalette::WindowText);
    const QColor gray = withAlpha(text, 120);
    QRectF plot = area.adjusted(30, 4, -4, -14);
    const double gap = 4;
    QRectF top(plot.left(), plot.top(), plot.width(), (plot.height() - gap) / 2);
    QRectF bottom(plot.left(), top.bottom() + gap, plot.width(), top.height());
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(withAlpha(text, 40), 1));
    p.drawRect(top);
    p.drawRect(bottom);
    p.setFont(pixelFont(font(), 10, false));
    p.setPen(withAlpha(text, 150));
    QString unit = result.with_current ? "A" : "% thr";
    // the units, over the lines (drawn last)
    auto units = [&]() {
        QFontMetricsF fm(p.font());
        for(auto [rect, label] : {std::pair<QRectF, QString>{top, "V/cell"}, {bottom, unit}}) {
            QRectF b(rect.left() + 1, rect.top() + 1, fm.horizontalAdvance(label) + 6, 12);
            p.setPen(Qt::NoPen);
            p.setBrush(withAlpha(palette().color(QPalette::Window), 220));
            p.drawRect(b);
            p.setPen(withAlpha(text, 150));
            p.drawText(b, Qt::AlignCenter, label);
        }
        p.setBrush(Qt::NoBrush);
    };
    p.drawText(QRectF(plot.left(), plot.bottom() + 1, plot.width(), 13), Qt::AlignLeft | Qt::AlignVCenter,
               QString::fromUtf8("\xE2\x88\x92%1 min").arg(qRound(HISTORY_S / 60)));
    p.drawText(QRectF(plot.left(), plot.bottom() + 1, plot.width(), 13), Qt::AlignRight | Qt::AlignVCenter, "now");
    if(history.size() < 2) {
        units();
        return;
    }

    // the scales: the voltage over its range, the load from 0
    // (the load to its 99th percentile: a single peak, at a take-off, does not flatten the rest)
    double vlo = 1e9, vhi = -1e9, lhi = 1;
    std::vector<double> loads;
    for(const auto &h : history) {
        vlo = std::min(vlo, h.cell);
        vhi = std::max(vhi, h.cell);
        lhi = std::max(lhi, h.load_x);
        loads.push_back(h.load);
    }
    auto k = loads.begin() + static_cast<long>(0.99 * (loads.size() - 1));
    std::nth_element(loads.begin(), k, loads.end());
    lhi = std::max(lhi, *k) * 1.1;
    if(vhi - vlo < 0.05) {
        double c = (vhi + vlo) / 2;
        vlo = c - 0.025;
        vhi = c + 0.025;
    }
    p.setPen(withAlpha(text, 150));
    p.drawText(QRectF(area.left(), top.top() - 2, 28, 12), Qt::AlignRight | Qt::AlignTop, QString::number(vhi, 'f', 2));
    p.drawText(QRectF(area.left(), top.bottom() - 10, 28, 12), Qt::AlignRight | Qt::AlignBottom, QString::number(vlo, 'f', 2));
    p.drawText(QRectF(area.left(), bottom.top() - 2, 28, 12), Qt::AlignRight | Qt::AlignTop, QString::number(qRound(lhi)));
    p.drawText(QRectF(area.left(), bottom.bottom() - 10, 28, 12), Qt::AlignRight | Qt::AlignBottom, "0");

    const double now = history.back().t;
    auto xof = [&](double t) { return plot.right() - plot.width() * (now - t) / HISTORY_S; };
    QPainterPath v, l, lx;
    for(size_t i = 0; i < history.size(); i++) {
        const auto &h = history[i];
        QPointF pv(xof(h.t), top.bottom() - top.height() * (h.cell - vlo) / (vhi - vlo));
        QPointF pl(xof(h.t), bottom.bottom() - bottom.height() * h.load / lhi);
        QPointF px(xof(h.t), bottom.bottom() - bottom.height() * h.load_x / lhi);
        if(i == 0) {
            v.moveTo(pv);
            l.moveTo(pl);
            lx.moveTo(px);
        } else {
            v.lineTo(pv);
            l.lineTo(pl);
            lx.lineTo(px);
        }
    }
    p.save();
    p.setClipRect(plot);
    p.setPen(QPen(gray, 1));
    p.drawPath(v);
    p.drawPath(l);
    p.setPen(QPen(ACCENT, 1.5));
    p.drawPath(lx);
    p.restore();
    units();
}

/// the names of the alarm levels, over the curves, on a light background
void EnergyViewer::drawLevelLabels(QPainter &p)
{
    p.save();
    p.setFont(pixelFont(font(), 9, false));
    for(const auto &l : level_labels) {
        p.setPen(Qt::NoPen);
        p.setBrush(withAlpha(palette().color(QPalette::Window), 210));
        p.drawRect(l.rect.adjusted(-2, 0, 2, 0));
        p.setPen(l.color);
        p.drawText(l.rect, Qt::AlignLeft | Qt::AlignVCenter, l.text);
    }
    p.restore();
}

/// The discharge curve: cell voltage at rest (and under the present load) against the energy used
void EnergyViewer::drawCurve(QPainter &p, QRectF area)
{
    const QColor text = palette().color(QPalette::WindowText);
    const auto chem = estimator.getChemistry();
    double vmin, vmax;
    plotRange(chem, vmin, vmax);
    QRectF plot = area.adjusted(30, 4, -4, -16);
    auto pos = [&](double energy_used, double v) {
        return QPointF(plot.left() + plot.width() * energy_used,
                       plot.bottom() - plot.height() * (v - vmin) / (vmax - vmin));
    };

    // grid, the reserve, labels
    p.setPen(QPen(withAlpha(text, 40), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(plot);
    p.setFont(pixelFont(font(), 10, false));
    double step = vmax - vmin > 1.0 ? 0.2 : 0.1;
    for(double v = vmin + step; v < vmax - 1e-6; v += step) {
        p.setPen(QPen(withAlpha(text, 40), 1));
        p.drawLine(pos(0, v), pos(1, v));
        p.setPen(withAlpha(text, 150));
        p.drawText(QRectF(area.left(), pos(0, v).y() - 7, 28, 14), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(v, 'f', 1));
    }
    p.setPen(withAlpha(text, 150));
    p.drawText(QRectF(plot.left() + 4, plot.top() + 2, 60, 12), Qt::AlignLeft | Qt::AlignTop, "V/cell");
    double reserve = estimator.getReserve();
    p.setPen(Qt::NoPen);
    p.setBrush(withAlpha(EMPTY, 30));
    p.drawRect(QRectF(pos(1 - reserve, vmax), pos(1, vmin)));
    p.setPen(withAlpha(text, 150));
    p.drawText(QRectF(plot.left(), plot.bottom() + 2, plot.width(), 14), Qt::AlignLeft | Qt::AlignVCenter, "full");
    p.drawText(QRectF(plot.left(), plot.bottom() + 2, plot.width(), 14), Qt::AlignHCenter | Qt::AlignVCenter, "energy used");
    p.drawText(QRectF(plot.left(), plot.bottom() + 2, plot.width(), 14), Qt::AlignRight | Qt::AlignVCenter, "empty");

    // the curves: at rest, and under the present load (dashed)
    auto curve = [&](double sag) {
        QPainterPath path;
        for(int i = 0; i <= 100; i++) {
            // energy used i %: find the charge of that energy left
            double e = 1 - i / 100.;
            double lo = 0, hi = 1;
            for(int k = 0; k < 30; k++) {
                double mid = (lo + hi) / 2;
                (EnergyEstimator::energyOfSoc(chem, mid) < e ? lo : hi) = mid;
            }
            QPointF pt = pos(i / 100., qMax(vmin, EnergyEstimator::ocvOfSoc(chem, lo) - sag));
            i == 0 ? path.moveTo(pt) : path.lineTo(pt);
        }
        return path;
    };
    // the alarm levels of the airframe (pack voltage over the cells): the measured voltage under load
    // reaches them, so they are compared with the gray trace and the battery under load
    if(estimator.getCells() > 0) {
        p.save();
        p.setClipRect(plot);
        p.setFont(pixelFont(font(), 9, false));
        QFontMetricsF lfm(p.font());
        struct Level { double v; QColor color; const char *name; const char *define; };
        const Level levels[] = {{level_low, LOW, "low", "LOW_BAT_LEVEL"},
                                {level_critic, EMPTY, "critic", "CRITIC_BAT_LEVEL"},
                                {level_catastrophic, withAlpha(text, 220), "catastrophic", "CATASTROPHIC_BAT_LEVEL"}};
        // the labels on the left (the battery is on the right, near empty), side by side when
        // the levels are close (low and critic often are)
        QRectF previous;
        level_labels.clear();
        auto &labels = level_labels;
        for(const auto &l : levels) {
            double pack = l.v;
            double v = pack / estimator.getCells();
            if(pack <= 0) {
                continue;
            }
            if(v <= vmin || v >= vmax) {
                continue;
            }
            double y = pos(0, v).y();
            p.setPen(QPen(l.color, 1, Qt::DashLine));
            p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
            QString label = QString("%1 %2 V%3").arg(l.name).arg(pack, 0, 'f', 1).arg(level_defaults.contains(l.define) ? " (default)" : "");
            QRectF lr(plot.left() + 4, y - 11, lfm.horizontalAdvance(label) + 2, 11);
            if(!previous.isNull() && lr.intersects(previous)) {
                lr.moveLeft(previous.right() + 8);
            }
            labels.push_back({lr, label, l.color});
            previous = lr;
        }
        p.restore();
    }

    const auto &r = result;
    // under the curves: the measured voltage over the flight, against the energy used estimated then
    if(r.valid) {
        QPainterPath measured;
        bool drawing = false;
        for(const auto &tr : trace) {
            double e = tr.energy[r.method];
            if(e < 0) {
                drawing = false;
                continue;
            }
            QPointF pt = pos(1 - e, qBound(vmin, tr.cell, vmax));
            drawing ? measured.lineTo(pt) : measured.moveTo(pt);
            drawing = true;
        }
        p.save();
        p.setClipRect(plot);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(withAlpha(text, 110), 1));
        p.drawPath(measured);
        p.restore();
    }
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(withAlpha(text, 200), 2));
    p.drawPath(curve(0));
    if(!r.valid) {
        return;
    }
    // the curve under load: minus the dV of the estimator (none for mAh)
    if(r.dv > 0.005) {
        p.setPen(QPen(ACCENT, 2));
        p.drawPath(curve(r.dv));
    }

    drawLevelLabels(p);

    // the legend, top right (the curves are low on the empty side)
    {
        struct Entry { QColor color; QString name; double width; };
        std::vector<Entry> entries = {{withAlpha(text, 200), "at rest (estimated)", 2}};
        if(r.dv > 0.005) {
            entries.push_back({ACCENT, "under load", 2});
        }
        if(trace.size() > 1) {
            entries.push_back({withAlpha(text, 110), "measured", 1});
        }
        p.save();
        p.setFont(pixelFont(font(), 9, false));
        QFontMetricsF lfm(p.font());
        double w = 0;
        for(const auto &e : entries) {
            w = std::max(w, lfm.horizontalAdvance(e.name));
        }
        const double line = 14, row = 11;
        QRectF box(plot.right() - w - line - 10, plot.top() + 3, w + line + 8, row * entries.size() + 4);
        p.setPen(Qt::NoPen);
        p.setBrush(withAlpha(palette().color(QPalette::Window), 210));
        p.drawRect(box);
        for(size_t i = 0; i < entries.size(); i++) {
            double y = box.top() + 2 + row * i + row / 2;
            p.setPen(QPen(entries[i].color, entries[i].width));
            p.drawLine(QPointF(box.left() + 3, y), QPointF(box.left() + 3 + line, y));
            p.setPen(withAlpha(text, 170));
            p.drawText(QRectF(box.left() + line + 6, y - row / 2, w + 2, row), Qt::AlignLeft | Qt::AlignVCenter, entries[i].name);
        }
        p.restore();
    }

    // the battery: at rest on the curve, under load below
    QPointF rest = pos(1 - r.energy, r.ocv);
    QPointF loaded = pos(1 - r.energy, qBound(vmin, r.cell, vmax));
    QColor level = r.energy <= reserve ? EMPTY : r.energy <= reserve + 0.2 ? LOW : GOOD;
    p.setPen(QPen(level, 2));
    p.drawLine(rest, loaded);
    p.setPen(Qt::NoPen);
    p.setBrush(level);
    p.drawEllipse(rest, 5, 5);
    p.setBrush(withAlpha(level, 140));
    p.drawEllipse(loaded, 3.5, 3.5);
}

void EnergyViewer::saveConfig(QString name, QVariant value)
{
    getAppSettings().setValue("energy/" + ac_name + "/" + name, value);
    result = estimator.estimate();
    update();
}

void EnergyViewer::askSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QString("Battery of %1").arg(ac_name));
    auto form = new QFormLayout(&dialog);

    auto chem = new QComboBox(&dialog);
    for(int i = 0; i < EnergyEstimator::CHEMISTRIES; i++) {
        chem->addItem(chemistryName(static_cast<EnergyEstimator::Chemistry>(i)));
    }
    chem->setCurrentIndex(estimator.getChemistry());
    chem->setToolTip(airframe_chemistry_set ? "Airframe (BAT_CHEMISTRY): " + chemistryName(airframe_chemistry)
                                             : QString("No BAT_CHEMISTRY in the airframe: LiPo by default"));
    form->addRow("Chemistry", chem);

    auto cells = new QSpinBox(&dialog);
    cells->setRange(0, 24);
    cells->setSpecialValueText("from the voltage");
    cells->setSuffix(" S");
    cells->setValue(cells_known ? estimator.getCells() : 0);
    form->addRow("Cells in series", cells);

    auto capacity = new QSpinBox(&dialog);
    capacity->setRange(0, 1000000);
    capacity->setSingleStep(100);
    capacity->setSpecialValueText("unknown");
    capacity->setSuffix(" mAh");
    capacity->setValue(qRound(estimator.getCapacity()));
    capacity->setToolTip("Capacity of the battery in use: change it when you change the battery, also in flight");
    if(airframe_capacity > 0) {
        // the airframe gives the usual battery; the one in use may differ
        auto row = new QWidget(&dialog);
        auto h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        h->addWidget(capacity, 1);
        auto reset = new QPushButton(QString("Airframe: %1 mAh").arg(qRound(airframe_capacity)), row);
        reset->setToolTip("BAT_CAPACITY of the airframe");
        connect(reset, &QPushButton::clicked, capacity, [capacity, cap = airframe_capacity]() { capacity->setValue(qRound(cap)); });
        h->addWidget(reset);
        form->addRow("Capacity", row);
    } else {
        form->addRow("Capacity", capacity);
    }

    auto resistance = new QDoubleSpinBox(&dialog);
    resistance->setRange(0, 100);
    resistance->setDecimals(1);
    resistance->setSingleStep(0.1);
    resistance->setSpecialValueText("learned from 0");
    resistance->setSuffix(QString::fromUtf8(" m\xCE\xA9"));
    resistance->setValue(estimator.getStartResistance() * 1000);
    resistance->setToolTip("Internal resistance of a cell, the start of R for V \xE2\x88\x92 I\xC2\xB7R (with a current sensor);\n"
                           "it is learned further in flight. Used from the next battery, or at once while R is not known yet");
    if(airframe_resistance > 0) {
        auto row = new QWidget(&dialog);
        auto h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        h->addWidget(resistance, 1);
        auto reset = new QPushButton(QString::fromUtf8("Airframe: %1 m\xCE\xA9").arg(airframe_resistance, 0, 'f', 1), row);
        reset->setToolTip("BAT_CELL_RESISTANCE of the airframe");
        connect(reset, &QPushButton::clicked, resistance, [resistance, r = airframe_resistance]() { resistance->setValue(r); });
        h->addWidget(reset);
        form->addRow("R per cell", row);
    } else {
        form->addRow("R per cell", resistance);
    }

    auto reserve = new QSpinBox(&dialog);
    reserve->setRange(0, 60);
    reserve->setSingleStep(5);
    reserve->setSuffix(" %");
    reserve->setValue(qRound(estimator.getReserve() * 100));
    form->addRow("Reserve", reserve);

    auto sag = new QDoubleSpinBox(&dialog);
    sag->setRange(0, 1);
    sag->setDecimals(2);
    sag->setSingleStep(0.01);
    sag->setSuffix(" V");
    sag->setValue(estimator.getSag());
    sag->setToolTip("Average sag of a cell under load, added to the voltage by the V estimator while the motors run (pill V + \xCE\x94V).\n"
                       "0: the voltage alone, low under load but never too high");
    form->addRow(QString::fromUtf8("\xCE\x94V under load per cell"), sag);

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);
    if(dialog.exec() != QDialog::Accepted) {
        return;
    }

    auto settings = getAppSettings();
    QString key = "energy/" + ac_name + "/";
    estimator.setChemistry(static_cast<EnergyEstimator::Chemistry>(chem->currentIndex()));
    if(chem->currentIndex() == airframe_chemistry) {
        settings.remove(key + "chemistry");
    } else {
        settings.setValue(key + "chemistry", chem->currentIndex());
    }
    estimator.setStartResistance(resistance->value() / 1000);
    if(std::abs(resistance->value() - airframe_resistance) < 0.05) {
        settings.remove(key + "resistance");
    } else {
        settings.setValue(key + "resistance", resistance->value());
    }
    bool was_known = cells_known;
    cells_known = cells->value() > 0;
    if(cells_known) {
        estimator.setCells(cells->value());
    } else if(was_known) {
        estimator.setCells(0);  // guessed again from the next voltage
    }
    if(cells->value() == airframe_cells) {
        settings.remove(key + "cells");
    } else {
        settings.setValue(key + "cells", cells->value());
    }
    estimator.setCapacity(capacity->value());
    if(capacity->value() == qRound(airframe_capacity)) {
        settings.remove(key + "capacity");
    } else {
        settings.setValue(key + "capacity", capacity->value());
    }
    estimator.setReserve(reserve->value() / 100.);
    settings.setValue(key + "reserve", reserve->value() / 100.);
    estimator.setSag(sag->value());
    settings.setValue(key + "sag", sag->value());
    result = estimator.estimate();
    update();
}

void EnergyViewer::mousePressEvent(QMouseEvent *event)
{
    QPointF pos = event->position();
    for(int i = 0; i < EnergyEstimator::METHODS; i++) {
        if(method_chips[i].contains(pos)) {
            estimator.setMethod(static_cast<EnergyEstimator::Method>(i));
            saveConfig("estimator", i);
            return;
        }
    }
    if(battery_line.contains(pos)) {
        askSettings();
        return;
    }
    QWidget::mousePressEvent(event);
}

void EnergyViewer::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(QString::fromUtf8("Battery\xE2\x80\xA6"), this, &EnergyViewer::askSettings);
    menu.addSeparator();
    menu.addAction("New battery (forget the samples)", this, [this]() {
        estimator.clear();
        if(!cells_known) {
            estimator.setCells(0);  // guessed again from the new battery
        }
        seen_current = false;
        history.clear();
        trace.clear();
        result = EnergyEstimator::Result();
        update();
    });
    menu.exec(event->globalPos());
}
