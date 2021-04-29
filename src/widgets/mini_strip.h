#ifndef MINISTRIP_H
#define MINISTRIP_H

#include <QtWidgets>
#include <QGridLayout>
#include <QHBoxLayout>
#include "colorlabel.h"

class MiniStrip : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int size READ getSizeProperty WRITE setSizeProperty DESIGNABLE true)
public:
    explicit MiniStrip(QString ac_id, QWidget *parent = nullptr);
    void setSizeProperty(int s) {icons_size = QSize(s, s);}
    int getSizeProperty() {return icons_size.width();}

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;


signals:
    void updated();
private:

    void updateData();

    void updateFlightTime(uint32_t flight_time);
    void updateAp(QString ap_mode);
    void updateGps(QString gps_mode);
    void updateImu(QString state_filter_mode);
    void updateCurrentBlock(uint8_t cur_block);

    QString ac_id;
    QSize icons_size;

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
    QIcon gps_ok;
    QIcon gps_lost;

    QLabel* rc_icon;
    QIcon rc_ok;
    QIcon rc_lost;

    QLabel* imu_icon;
    QIcon imu_ok;
    QIcon imu_lost;

};

#endif // MINISTRIP_H
