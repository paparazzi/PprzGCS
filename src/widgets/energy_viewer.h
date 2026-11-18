#ifndef ENERGY_VIEWER_H
#define ENERGY_VIEWER_H

#include <QWidget>
#include <QElapsedTimer>
#include <deque>
#include <vector>
#include <QToolButton>
#include "energy_estimator.h"

/**
 * Energy left in the battery of an aircraft (see EnergyEstimator), after the energy_mon tool of
 * Paparazzi: the energy left in % of Wh with a bar, the cell voltage under load and at rest, the
 * discharge curve of the chemistry with the state of the battery on it (at rest on the curve,
 * under load below it), and under it the charge left, the flight time to the reserve and the
 * internal resistance. Inputs: the voltage, and the current when measured, else the throttle.
 * The chemistry, cells, capacity and R come from the airframe (BAT_CHEMISTRY, BAT_NB_CELLS,
 * BAT_CAPACITY in mAh, BAT_CELL_RESISTANCE in mOhm per cell).
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
    void resizeEvent(QResizeEvent *event) override;

private:
    void addEngineStatus();
    void drawCurve(QPainter &p, QRectF area);
    /// V - I.R: the history of the measured voltage and load, and the filtered load
    void drawHistory(QPainter &p, QRectF area);
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
    int airframe_cells = 0;         // BAT_NB_CELLS of the airframe; 0: none
    double airframe_capacity = 0;   // mAh, BAT_CAPACITY of the airframe; 0: none
    int airframe_chemistry = -1;    // BAT_CHEMISTRY of the airframe; -1: none (LiPo)
    double airframe_resistance = 0; // mOhm per cell, BAT_CELL_RESISTANCE of the airframe; 0: none
    QRectF method_chips[EnergyEstimator::METHODS];
    QRectF battery_line;
    struct Past {
        double t;       // s
        double cell;    // V per cell, measured
        double load;    // A, else % of throttle
        double load_x;  // the same, low-passed
    };
    std::deque<Past> history;   // the last minutes, for drawHistory
    struct Trace {
        double t;       // s
        double cell;    // V per cell, measured
        double energy[EnergyEstimator::METHODS];    // left, 0..1, by each method; -1: unknown
    };
    std::vector<Trace> trace;   // the whole flight (1 per s), drawn on the curve
    QToolButton *help_button;  // the help, as in the app settings
};

#endif // ENERGY_VIEWER_H
