#include "energy_viewer.h"
#include "AircraftManager.h"
#include "gcs_utils.h"
#include <QComboBox>
#include <QContextMenuEvent>
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
#include <QDoubleSpinBox>
#include <cmath>

namespace {

const int MIN_WIDTH = 260;
const double CHIP_H = 20;
const double CHIPS = CHIP_H + 6;        // the pills of the estimator
const double HEADER = CHIPS + 76;
const double CURVE_HEIGHT = 130;
const double FOOTER = 36;               // charge, time and resistance under the plot

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
            estimator.setCells(cells->value.toInt());
            cells_known = true;
        }
        auto cap = ac->getAirframe()->getDefine("MAX_BAT_CAPACITY");
        if(cap && cap->value.toDouble() > 0) {
            airframe_capacity = cap->value.toDouble();
            estimator.setCapacity(airframe_capacity);
        }
    }
    // the choices of the user, per aircraft, over the airframe
    auto settings = getAppSettings();
    QString key = "energy/" + ac_name + "/";
    estimator.setChemistry(static_cast<EnergyEstimator::Chemistry>(
        qBound(0, settings.value(key + "chemistry", 0).toInt(), EnergyEstimator::CHEMISTRIES - 1)));
    estimator.setMethod(static_cast<EnergyEstimator::Method>(
        qBound(0, settings.value(key + "estimator", EnergyEstimator::SAG).toInt(), EnergyEstimator::METHODS - 1)));
    if(settings.contains(key + "cells")) {
        estimator.setCells(settings.value(key + "cells").toInt());
        cells_known = true;
    }
    if(settings.contains(key + "capacity")) {
        estimator.setCapacity(settings.value(key + "capacity").toDouble());
    }
    estimator.setReserve(settings.value(key + "reserve", 0.2).toDouble());
    estimator.setSag(settings.value(key + "sag", 0.0).toDouble());
    clock.start();

    connect(ac->getStatus(), &AircraftStatus::engine_status, this, &EnergyViewer::addEngineStatus);
    setToolTip("Energy left (Wh) from the voltage, corrected for the sag under load\n"
               "(the current when measured, else the throttle).\n"
               "V: the voltage + \xCE\x94V set in Battery\xE2\x80\xA6 (while the motors run).\n"
               "mAh: the charge used (sent by the aircraft, else the current counted) over the capacity.\n"
               "V \xE2\x88\x92 I\xC2\xB7R: the voltage + 60 % of R \xC3\x97 load, R learned from the changes of the load.\n"
               "Filter: the mAh count, pulled slowly towards V \xE2\x88\x92 I\xC2\xB7R.\n"
               "Click the battery line or right click: chemistry, cells, capacity, reserve.");
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
    update();
}

QSize EnergyViewer::sizeHint() const
{
    return QSize(300, minimumSizeHint().height());
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
                                            : "No current nor mAh in ENGINE_STATUS: nothing to count";
        hint += m == EnergyEstimator::FILTER ? "; V \xE2\x88\x92 I\xC2\xB7R meanwhile" : "; V meanwhile";
    } else if(r.valid && (r.method == EnergyEstimator::SAG || r.method == EnergyEstimator::FILTER) && !r.r_known) {
        hint = r.with_current ? "R not known yet: change the current" : "R not known yet: change the throttle";
    }
    double curve_top = y + 74;
    if(!hint.isEmpty()) {
        p.setPen(LOW);
        p.drawText(QRectF(0, y + 72, width(), 14), Qt::AlignLeft | Qt::AlignVCenter, hint);
        curve_top += 12;
    }

    double curve_h = qMax(CURVE_HEIGHT, height() - curve_top - FOOTER - 2);
    drawCurve(p, QRectF(0, curve_top, width(), curve_h));

    // under the plot: the charge left, the flight time to the reserve, and what the estimator uses
    QRectF foot(0, curve_top + curve_h + 2, width(), FOOTER);
    double cw = foot.width() / 3;
    auto stat = [&](int i, QString caption, QString value) {
        QRectF c(foot.left() + i * cw, foot.top(), cw, foot.height());
        p.setFont(pixelFont(font(), 10, false));
        p.setPen(secondary);
        p.drawText(QRectF(c.left(), c.top(), c.width(), 13), Qt::AlignLeft | Qt::AlignVCenter, caption);
        p.setFont(pixelFont(font(), 15, true));
        p.setPen(r.valid ? text : secondary);
        p.drawText(QRectF(c.left(), c.top() + 13, c.width(), 20), Qt::AlignLeft | Qt::AlignVCenter, value);
    };
    stat(0, "Charge left", r.valid ? QString("%1 %").arg(qRound(r.soc * 100)) : none);
    stat(1, QString("Time to %1 %").arg(qRound(reserve * 100)),
         r.minutes >= 0 ? QString::fromUtf8("\xE2\x89\x88 %1 min").arg(r.minutes, 0, 'f', r.minutes < 10 ? 1 : 0) : none);
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
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(withAlpha(text, 200), 2));
    p.drawPath(curve(0));
    const auto &r = result;
    if(!r.valid) {
        return;
    }
    // the curve under load: minus the dV of the estimator (none for mAh)
    if(r.dv > 0.005) {
        p.setPen(QPen(ACCENT, 2));
        p.drawPath(curve(r.dv));
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
        reset->setToolTip("MAX_BAT_CAPACITY of the airframe");
        connect(reset, &QPushButton::clicked, capacity, [capacity, cap = airframe_capacity]() { capacity->setValue(qRound(cap)); });
        h->addWidget(reset);
        form->addRow("Capacity", row);
    } else {
        form->addRow("Capacity", capacity);
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
    settings.setValue(key + "chemistry", chem->currentIndex());
    bool was_known = cells_known;
    cells_known = cells->value() > 0;
    if(cells_known) {
        estimator.setCells(cells->value());
        settings.setValue(key + "cells", cells->value());
    } else {
        settings.remove(key + "cells");
        if(was_known) {
            estimator.setCells(0);  // guessed again from the next voltage
        }
    }
    estimator.setCapacity(capacity->value());
    settings.setValue(key + "capacity", capacity->value());
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
        result = EnergyEstimator::Result();
        update();
    });
    menu.exec(event->globalPos());
}
