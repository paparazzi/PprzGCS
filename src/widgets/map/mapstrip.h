#ifndef MAPSTRIP_H
#define MAPSTRIP_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include "pprz_dispatcher.h"

namespace Ui {
class MapStrip;
}

class MapStrip : public QWidget
{
    Q_OBJECT

public:
    explicit MapStrip(QString ac_id, QWidget *parent = nullptr);
    ~MapStrip();

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);

private:
    void addFlightPlanButtons();
    void addSettingsButtons();

    void updateFlightParams(pprzlink::Message msg);
    void updateApStatus(pprzlink::Message msg);
    void updateNavStatus(pprzlink::Message msg);
    void updateEngineStatus(pprzlink::Message msg);

    void setExtended(bool visi);

    QString ac_id;

    bool lock;

    QHBoxLayout *layout;
    QGridLayout *info_layout;
    QGridLayout *fp_buttons_layout;
    QGridLayout *settings_buttons_layout;
    QLabel* ac_name;
    QLabel* bat_label;
    QLabel* link_label;
    QLabel* speed_label;
    QLabel* height_label;

    QLabel* ap_mode;
    QLabel* cur_block;
};

#endif // MAPSTRIP_H
