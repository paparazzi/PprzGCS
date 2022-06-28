#include "gvf_trajectory.h"

GVF_trajectory::GVF_trajectory(QString id, Point2DLatLon origin)
{
    ac_id = id;
    ltp_origin = origin;
}

QList<QPointF> GVF_trajectory::meshGrid(float area, int xpoints_num, int ypoints_num)
{
    QList<QPointF> grid;

    float dx = sqrt(area)/(xpoints_num - 1);
    float dy = sqrt(area)/(ypoints_num - 1);
    for(float x=xy_off.x() - 0.5*sqrt(area); x<=xy_off.x() + 0.5*sqrt(area) + dx/2; x+=dx) {
        for(float y=xy_off.y() - 0.5*sqrt(area); y<=xy_off.y() + 0.5*sqrt(area) + dy/2; y+=dy) {
            grid.append(QPointF(x,y));
        }
    }

    return grid;
}

PathItem* GVF_trajectory::getTraj() {
    return traj_item;
}

QuiverItem* GVF_trajectory::getVField() {
    return vector_field;
}

// We have to delete previous waypoints if we don't want to cause memory overflow!!
void GVF_trajectory::delete_waypoints() { 
    foreach (WaypointItem* wp, traj_waypoints) {
        assert(wp != nullptr);
        delete wp;
    }
}

void GVF_trajectory::createTrajItem(QList<QPointF> points) // TODO
{
    //auto color = AircraftManager::get()->getAircraft(ac_id)->getColor();
    auto color = Qt::green;

    traj_item = new PathItem(ac_id, color);
    //traj_item->setClosedPath(true);
    //traj_item->setZValues(z, z);

    for(auto point: points) {
        auto pos = CoordinatesTransform::get()->relative_utm_to_wgs84(ltp_origin, point.x(), point.y());
        auto wp =  new WaypointItem(pos, ac_id, color); 
        traj_item->addPoint(wp);
        traj_waypoints.append(wp);
    }

    traj_item->setText("AC " + ac_id + " GVF");
}

void GVF_trajectory::createVFieldItem(QList<QPointF> points, QList<QPointF> vpoints, float bound_area) 
{   
    QList<Point2DLatLon> pos;
    QList<Point2DLatLon> vpos;

    float scale = sqrt(150/bound_area); // arrows scaling based on the trajectory bounding area
    for (int i=0; i<points.size(); i++) {
        float vx = vpoints[i].x();
        float vy = vpoints[i].y();

        float renorm = sqrt(pow(vx,2) + pow(vy,2))*scale;
        vx = vx/renorm;
        vy = vy/renorm;

        pos.append(CoordinatesTransform::get()->relative_utm_to_wgs84(ltp_origin, points[i].x(), points[i].y()));
        vpos.append(CoordinatesTransform::get()->ltp_to_wgs84(pos[i], vx, vy));
    }
    
    //auto color = AircraftManager::get()->getAircraft(ac_id)->getColor();
    auto color = Qt::red;
    vector_field = new QuiverItem(pos, vpos, ac_id, color, 0.5);
}