#ifndef GVF_TRAJECTORY_H
#define GVF_TRAJECTORY_H

#include "coordinatestransform.h"
#include "AircraftManager.h"
#include "quiver_item.h"
#include "path_item.h"
#include <math.h>
#include <QPointF>
#include <QList>

#include <QDebug>

constexpr int QUIVERS_NUMBER = 20;
constexpr float DEFAULT_AREA = 1000;

class GVF_trajectory
{
public:
    GVF_trajectory(QString id, QPointF pos, Point2DLatLon origin);

    QList<QPointF> meshGrid(float area = DEFAULT_AREA, int xpoints_num = QUIVERS_NUMBER, int ypoints_num = QUIVERS_NUMBER);

    void createTrajItem(QList<QPointF> points);
    void createVFieldItem(QList<QPointF> points, QList<QPointF> vpoints, float bound_area);

    QuiverItem* getVField();
    PathItem* getTraj();

    void delete_waypoints();
    
protected:
    QString ac_id;
    Point2DLatLon ltp_origin;
    QPointF ac_pos;
    QPointF xy_off;
    QPointF traj_grad;

    QList<QPointF> xy_mesh;
    QList<QPointF> vxy_mesh;  

private:
    QuiverItem* vector_field;
    PathItem* traj_item;
    QList<WaypointItem*> traj_waypoints;

};

#endif // GVF_TRAJECTORY_H