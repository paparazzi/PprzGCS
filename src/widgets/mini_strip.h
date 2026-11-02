#ifndef MINISTRIP_H
#define MINISTRIP_H

#include <QtWidgets>
#include <QGridLayout>
#include <QHBoxLayout>
#include "aircraft_watcher.h"

class MiniStrip : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int size READ getSizeProperty WRITE setSizeProperty DESIGNABLE true)
public:
    explicit MiniStrip(QString ac_id, QWidget *parent = nullptr);
    void setSizeProperty(int s) {icons_size = QSize(s, s);}
    int getSizeProperty() {return icons_size.width();}

public slots:
    /// smaller: the values without their captions (e.g. for the aircraft not selected, when there
    /// are many)
    void setCompact(bool compact);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;     // center the map on it


signals:
    void updated();
private:

    static const QMap<QString, QString> mode_colors;

    enum SpeedMode {
        GROUND_SPEED,
        AIR_SPEED,
        VERTICAL_SPEED,
    };

    enum TimeMode {
        FLIGHT_TIME,
        BLOCK_TIME,
        STAGE_TIME,
    };

    void handle_bat_status(AircraftWatcher::BatStatus);

    void updateData();

    void updateFlightTime(uint32_t flight_time, uint32_t block_time, uint32_t stage_time);
    void updateAp(QString ap_mode);
    void updateGps(QString gps_mode);
    void updateImu(QString state_filter_mode);
    void updateCurrentBlock(uint8_t cur_block);
    void updateStatusRow();
    void centerOnAircraft();
    QPushButton* makeCell(QLabel*& caption, QLabel*& value, bool vertical);

    QString ac_id;
    QSize icons_size;
    QList<QLabel*> captions;
    QPushButton* kill_button = nullptr;    // kill / resurrect, with a confirmation
    QList<QPushButton*> value_cells;
    QHBoxLayout* status_layout;
    QHBoxLayout* compact_icons;     // the status icons on the first line, when compact
    bool compact = false;

    QPushButton* time_button;
    QLabel* flight_time_label;
    TimeMode time_mode;

    QPushButton* alt_button;
    QLabel* alt_label;
    bool alt_mode;

    QPushButton* speed_button;
    QLabel* speed_label;
    SpeedMode speed_mode;

    QLabel* block_label;
    QLabel* block_icon;

    QLabel* bat_label;
    int bat_nb_cells;

    QLabel* throttle_caption;
    QLabel* throttle_label;

    QPushButton* ap_mode_button;

    QLabel* link_icon;
    QIcon link_ok;
    QIcon link_warning;
    QIcon link_lost;

    QLabel* gps_icon;
    QIcon gps_lost;
    QIcon gps_ok;
    QIcon gps_dgps;
    QIcon gps_rtk;

    QLabel* rc_icon;
    QIcon rc_ok;
    QIcon rc_lost;

    QLabel* imu_icon;
    QIcon imu_ok;
    QIcon imu_lost;

    QWidget* status_row;    // the status icons that are not ok
    QLabel* status_text;

};

#endif // MINISTRIP_H
