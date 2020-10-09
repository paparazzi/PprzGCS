#ifndef FLIGHTPLAN_VIEWERV2_H
#define FLIGHTPLAN_VIEWERV2_H

#include <QTabWidget>
#include <block.h>
#include <memory>
#include <functional>

class FlightPlanViewerV2 : public QTabWidget
{
    Q_OBJECT
public:
    explicit FlightPlanViewerV2(QString ac_id, QWidget *parent = nullptr);

signals:

public slots:

private:
    QWidget* make_blocks_tab();
    QWidget* make_waypoints_tab();
    QWidget* make_tree(shared_ptr<Block> block, std::function<void()>);

    QString ac_id;
};

#endif // FLIGHTPLAN_VIEWERV2_H
