#ifndef STRIP_H
#define STRIP_H

#include <QtWidgets>
#include <QMouseEvent>
#include "pprz_dispatcher.h"
#include "graphlabel.h"
#include "jaugelabel.h"
#include "colorlabel.h"

class Strip : public QWidget
{
    Q_OBJECT
public:
    explicit Strip(QString ac_id, QWidget *parent = nullptr, bool full = false);

    void setCompact(bool);

signals:

public slots:

protected:
    void paintEvent(QPaintEvent*) override;
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);

private:
    void build_full_strip();
    void build_short_strip();

    void updateEngineStatus(pprzlink::Message msg);
    void updateApStatus(pprzlink::Message msg);
    void updateFlightParams(pprzlink::Message msg);
    void updateTelemetryStatus(pprzlink::Message msg);
    void updateFBW(pprzlink::Message msg);

    QString _ac_id;

    QWidget* full_strip;
    QWidget* short_strip;

    GraphLabel* full_bat_graph;
    GraphLabel* full_alt_graph;
    JaugeLabel* full_throttle_label;
    JaugeLabel* full_speed_label;
    ColorLabel* full_link_label;
    ColorLabel* full_ap_mode_label;
    ColorLabel* full_fbw_mode_label;
    ColorLabel* full_gps_mode_label;
    QLabel*     full_flight_time_label;


    JaugeLabel* short_jl_bat;
    QLabel*     short_flight_time_label;
    QLabel*     short_speed_label;
    QLabel*     short_alt_label;
    QLabel*     short_vspeed_indicator;
    QLabel*     short_vspeed_label;
    QLabel*     short_target_label;
    ColorLabel* short_ap_mode_label;
    ColorLabel* short_fbw_mode_label;
    ColorLabel* short_gps_mode_label;
};

#endif // STRIP_H
