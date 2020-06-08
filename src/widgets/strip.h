#ifndef STRIP_H
#define STRIP_H

#include <QtWidgets>
#include "pprz_dispatcher.h"
#include "graphlabel.h"
#include "jaugelabel.h"
#include "colorlabel.h"

class Strip : public QWidget
{
    Q_OBJECT
public:
    explicit Strip(QString ac_id, QWidget *parent = nullptr);

signals:

public slots:

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void updateEngineStatus(pprzlink::Message msg);
    void updateApStatus(pprzlink::Message msg);
    void updateFlightParams(pprzlink::Message msg);
    void updateTelemetryStatus(pprzlink::Message msg);
    void updateFBW(pprzlink::Message msg);

    QString _ac_id;
    QVBoxLayout* layout;
    QVBoxLayout* lay_bat_link;
    QHBoxLayout* lay_head;
    QHBoxLayout* lay_body;
    QVBoxLayout* lay_status;
    GraphLabel* bat_graph;
    GraphLabel* alt_graph;
    JaugeLabel* throttle_label;
    JaugeLabel* speed_label;
    ColorLabel* link_label;
    ColorLabel* ap_mode_label;
    ColorLabel* fbw_mode_label;
    ColorLabel* gps_mode_label;
    QLabel* flight_time_label;
};

#endif // STRIP_H
