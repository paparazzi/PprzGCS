#ifndef ATC_PANEL_H
#define ATC_PANEL_H

#include <QWidget>
#include <QElapsedTimer>
#include <map>
#include "pprz_dispatcher.h"
#include "configurable.h"

class QTimer;

/**
 * What the pilot tells ATC, for the selected aircraft, in big numbers and aviation units, top to
 * bottom in the order of the radio call ("... Tower, Bebop2, uncontrolled fly away, heading 030,
 * 30 knots, 500 feet"):
 *  - name of the aircraft (as callsign)
 *  - heading of the ground track, 3 digits, rounded to 5 deg
 *  - ground speed in kt
 *  - altitude in ft on QNH or QFE, or flight level (click to choose; right click to set the QNH);
 *    QNH: set in the GCS, else from the aircraft, else estimated from the pressure and GPS altitude
 *  - flight time in min, battery
 *  - a timer (e.g. started at a fly away), with the distance flown since, from the ground speed
 *    (one per aircraft)
 *  - bearing and distance from the ARP waypoint (aerodrome reference point), else HOME
 *  - distances in NM, or in m (km from 1000 m): click to choose
 */
class AtcPanel : public QWidget, public Configurable
{
    Q_OBJECT
public:
    explicit AtcPanel(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    void configure(QDomElement) override {};

    enum AltRef {QNH, QFE, FL};

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    struct AirData {
        float pressure = 0;     // Pa
        float qnh = 0;          // hPa, from the aircraft
        float qfe = 0;          // Pa, at the origin (INS_REF)
        float qnh_est = 0;      // hPa, estimated here: pressure matched to the GPS altitude (MSL)
        QElapsedTimer last;     // last pressure
    };

    void tick();
    void setAltRef(AltRef ref);
    void setDistMetric(bool metric);
    QString distText(double d) const;
    void askQnh();

    QString current_ac;
    std::map<QString, AirData> air_data;
    AltRef alt_ref = QNH;
    float qnh_set = 0;          // hPa, set in the GCS (0: the one of the aircraft, or 1013.25)
    bool dist_metric = false;   // distances in m / km, else in NM

    // a timer per aircraft, running until RESET (also when the aircraft is reloaded)
    struct Timer {
        QElapsedTimer clock;
        qint64 last_tick_ms = 0;
        double dist = 0;        // m
        float speed = 0;        // m/s, the last ground speed received
    };
    std::map<QString, Timer> timers;

    // clickable areas, from the last paint
    QRectF timer_button;
    QRectF ref_chips[3];
    QRectF dist_chips[2];       // NM, m
};

#endif // ATC_PANEL_H
