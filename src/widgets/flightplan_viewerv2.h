#ifndef FLIGHTPLAN_VIEWERV2_H
#define FLIGHTPLAN_VIEWERV2_H

#include <QtWidgets>
#include <block.h>
#include <memory>
#include <functional>
#include "pprz_dispatcher.h"

class FlightPlanViewerV2 : public QTabWidget
{
    Q_OBJECT
public:
    explicit FlightPlanViewerV2(QString ac_id, QWidget *parent = nullptr);

signals:

public slots:

private:
    void handleNavStatus();
    void updateNavStatus(uint8_t cur_block, uint8_t cur_stage, uint32_t block_time);

    QWidget* make_blocks_tab();
    QString ac_id;
    uint8_t current_block, current_stage;

    QList<QPushButton*> block_rows;     // one per block: tap to select, then "Go"
    QList<QLabel*> block_times;         // time in the current block
};

#endif // FLIGHTPLAN_VIEWERV2_H
