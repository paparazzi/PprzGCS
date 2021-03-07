#ifndef MINISTRIP_H
#define MINISTRIP_H

#include <QtWidgets>
#include <QGridLayout>
#include <QHBoxLayout>
#include "colorlabel.h"

class MiniStrip : public QWidget
{
    Q_OBJECT
public:
    explicit MiniStrip(QString ac_id, QWidget *parent = nullptr);

private:

    void updateFBW();
    void updateTelemetryStatus();
    void updateFlightParams();
    void updateApStatus();
    void updateEngineStatus();
    void updateNavStatus();

    QString ac_id;

    QLabel* flight_time_label;

    QToolButton* alt_button;
    QIcon agl_icon;
    QIcon msl_icon;
    QLabel* alt_label;
    bool alt_mode;

    QToolButton* speed_button;
    QIcon ground_speed_icon;
    QIcon air_speed_icon;
    QLabel* speed_label;
    bool speed_mode;

    QLabel* block_label;
    QLabel* block_icon;

    QLabel* bat_icon;
    QIcon bat_full, bat_half, bat_low;
    QLabel* bat_label;

    QLabel* throttle_icon;
    QLabel* throttle_label;
    QIcon throttle_on;
    QIcon throttle_killed;

    ColorLabel* ap_mode_label;

    QLabel* link_icon;
    QIcon link_ok;
    QIcon link_lost;

    QLabel* gps_icon;
    QLabel* rc_icon;

};

#endif // MINISTRIP_H
