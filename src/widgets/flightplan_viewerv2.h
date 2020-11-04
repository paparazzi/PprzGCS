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
    void updateNavStatus(uint8_t cur_block, uint8_t cur_stage);

    struct Facade {
        QLabel* label;
        QTreeWidget* tree;
        QPushButton* button;
    };

    QWidget* make_blocks_tab();
    QWidget* make_waypoints_tab();
    QWidget* make_exceptions_tab();
    QWidget* make_variables_tab();
    QWidget* make_sectors_tab();
    QWidget* make_tree(shared_ptr<Block> block, std::function<void()>, struct Facade*);

    QString ac_id;
    uint8_t current_block, current_stage;

    QList<struct Facade*> facades;

    QString labels_stylesheet;
    QString buttons_stylesheet;
};

#endif // FLIGHTPLAN_VIEWERV2_H
