#ifndef GVF_TRAJECTORY_H
#define GVF_TRAJECTORY_H

#include "coordinatestransform.h"
#include "AircraftManager.h"
#include "quiver_item.h"
#include "path_item.h"

constexpr int QUIVERS_NUMBER = 20;
constexpr float DEFAULT_AREA = 1000;

class GVF_trajectory : public QObject
{
    Q_OBJECT
public:
    GVF_trajectory(QString id, Point2DLatLon origin, QList<float> gvf_settings);
    QList<QPointF> meshGrid(float area = DEFAULT_AREA, int xpoints_num = QUIVERS_NUMBER, int ypoints_num = QUIVERS_NUMBER);

    void createTrajItem(QList<QPointF> points);
    void createVFieldItem(QList<QPointF> points, QList<QPointF> vpoints, float bound_area, float ref_area = 150);

    QuiverItem* getVField();
    PathItem* getTraj();

    void setVFiledVis(bool vis);

    void purge_trajectory();
        
protected:
    QString ac_id;
    Point2DLatLon ltp_origin;
    QPointF xy_off;
    QPointF traj_grad;

    QList<QPointF> xy_mesh;
    QList<QPointF> vxy_mesh;  

private:
    QObject* child;

    QuiverItem* vector_field;
    PathItem* traj_item;
    QList<WaypointItem*> traj_waypoints;

    bool vector_field_vis;
    bool traj_item_vis;

};

#endif // GVF_TRAJECTORY_H