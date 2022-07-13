#ifndef GVF_TRAJECTORY_H
#define GVF_TRAJECTORY_H

#include <QVector3D>
#include "coordinatestransform.h"
#include "AircraftManager.h"
#include "dispatcher_ui.h"
#include "quiver_item.h"
#include "path_item.h"


class GVF_trajectory : public QObject
{
    Q_OBJECT
public:
    explicit GVF_trajectory(QString id, Point2DLatLon origin, QVector<int> *gvf_settings);

    QuiverItem* getVField();
    PathItem* getTraj();

    void setVFiledVis(bool vis);

    void purge_trajectory();
    void update_trajectory();

protected:
    QString ac_id;
    Point2DLatLon ltp_origin;
    QPointF xy_off;
    QPointF traj_grad;

    QList<QPointF> xy_mesh;
    QList<QPointF> vxy_mesh;

    void createTrajItem(QList<QPointF> points);
    void createVFieldItem(QList<QPointF> points, QList<QPointF> vpoints, float ref_area = 500);
    QList<QPointF> meshGrid();
    QPointF getACpos();

    virtual void genTraj() = 0;
    virtual void genVField() = 0;

    // GVF viewer config
    int field_area;
    int field_xpts;
    int field_ypts;
    
private:
    QuiverItem* field_item;
    PathItem* traj_item;
    QList<WaypointItem*> traj_waypoints;

    // GVF viewer config
    bool field_item_vis;
    bool traj_item_vis;


};

#endif // GVF_TRAJECTORY_H