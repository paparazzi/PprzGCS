#ifndef GVF_TRAJECTORY_H
#define GVF_TRAJECTORY_H

#include "coordinatestransform.h"
#include "AircraftManager.h"
#include "quiver_item.h"
#include "path_item.h"

const int QUIVERS_NUMBER = 20;
const int DEFAULT_AREA = 1000;

class GVF_trajectory : public QObject
{
    Q_OBJECT
public:
    explicit GVF_trajectory(QString id, Point2DLatLon origin, QList<float> gvf_settings);

    QuiverItem* getVField();
    PathItem* getTraj();

    void setVFiledVis(bool vis);

    void purge_trajectory();
    void update_trajectory();

    virtual void set_param(QList<float> param, int8_t _s, float _ke) = 0;

protected:
    QString ac_id;
    Point2DLatLon ltp_origin;
    QPointF xy_off;
    QPointF traj_grad;

    QList<QPointF> xy_mesh;
    QList<QPointF> vxy_mesh;

    void createTrajItem(QList<QPointF> points);
    void createVFieldItem(QList<QPointF> points, QList<QPointF> vpoints, float bound_area, float ref_area = 150);
    QList<QPointF> meshGrid(float area = 1000, int xpoints_num = 20, int ypoints_num = 20);
    QPointF getACpos();

    virtual void genTraj() = 0;
    virtual void genVField() = 0;

    // GVF viewer config (TODO)
    float field_area;
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