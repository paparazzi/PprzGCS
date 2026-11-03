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
 *  - heading of the ground track, 3 digits, rounded to 10 deg
 *  - ground speed in kt
 *  - altitude in ft on QNH, QFE or flight level (click to choose; right click to set the QNH)
 *  - flight time in min, battery
 *  - a timer (e.g. started at a fly away), with the distance flown since, from the ground speed
 *  - bearing and distance from HOME, in NM
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
        QElapsedTimer last;     // last pressure
    };

    void tick();
    void setAltRef(AltRef ref);
    void askQnh();

    QString current_ac;
    std::map<QString, AirData> air_data;
    AltRef alt_ref = QNH;
    float qnh_set = 0;          // hPa, set in the GCS (0: the one of the aircraft, or 1013.25)

    // timer
    bool timer_on = false;
    QElapsedTimer timer;
    qint64 last_tick_ms = 0;
    double timer_dist = 0;      // m
    float last_speed = 0;       // m/s, the last ground speed received

    // clickable areas, from the last paint
    QRectF timer_button;
    QRectF ref_chips[3];
};

#endif // ATC_PANEL_H
