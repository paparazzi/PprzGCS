#ifndef ENERGY_VIEWER_H
#define ENERGY_VIEWER_H

#include <QWidget>
#include <QElapsedTimer>
#include "energy_estimator.h"

/**
 * Energy left in the battery of an aircraft (see EnergyEstimator), after the energy_mon tool of
 * Paparazzi: the energy left in % of Wh with a bar, the cell voltage under load and at rest, the
 * discharge curve of the chemistry with the state of the battery on it (at rest on the curve,
 * under load below it), and under it the charge left, the flight time to the reserve and the
 * internal resistance. Inputs: the voltage, and the current when measured, else the throttle.
 * The cells and the capacity come from the airframe (BAT_NB_CELLS, MAX_BAT_CAPACITY in mAh).
 * Pills at the top: the estimator (V, V - I*R or filter), changed often in flight. The battery
 * (chemistry, cells, capacity, reserve) in a window: click the battery line, or right click.
 * All saved per aircraft.
 */
class EnergyViewer : public QWidget
{
    Q_OBJECT
public:
    explicit EnergyViewer(QString ac_id, QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void addEngineStatus();
    void drawCurve(QPainter &p, QRectF area);
    /// the pills of the estimator, at the top of the panel; returns their height
    double drawChips(QPainter &p);
    /// the window of the battery settings
    void askSettings();
    /// save a choice of the user (per aircraft) and estimate again
    void saveConfig(QString name, QVariant value);

    QString ac_id;
    QString ac_name;
    EnergyEstimator estimator;
    EnergyEstimator::Result result;
    QElapsedTimer clock;
    bool seen_current = false;      // the aircraft measures the current
    bool cells_known = false;       // from the airframe or the user, else guessed from the voltage
    double airframe_capacity = 0;   // mAh, MAX_BAT_CAPACITY of the airframe; 0: none
    QRectF method_chips[EnergyEstimator::METHODS];
    QRectF battery_line;
};

#endif // ENERGY_VIEWER_H
