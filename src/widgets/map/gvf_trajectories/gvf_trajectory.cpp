#include "gvf_trajectory.h"
#include "mapwidget.h"
#include "gcs_utils.h"

GVF_trajectory::GVF_trajectory(uint8_t id, QPointF pos, Point2DLatLon origin)
{
    ac_id = id;
    ac_pos = pos;
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

void GVF_trajectory::delete_waypoints() { // We have to delete this objetcs!!
    foreach (WaypointItem* wp, traj_waypoints) {
        assert(wp != nullptr);
        delete wp;
    }
}

void GVF_trajectory::createTrajItem(QList<QPointF> points) // TODO
{
    //auto color = AircraftManager::get()->getAircraft(QString::number(ac_id))->getColor();
    auto color = Qt::green;

    traj_item = new PathItem(QString::number(ac_id), color);
    traj_item->setClosedPath(true);
    //traj_item->setZValues(z, z);

    for(auto point: points) {
        auto pos = CoordinatesTransform::get()->ltp_to_wgs84(ltp_origin, point.x(), point.y());
        auto wp =  new WaypointItem(pos, QString::number(ac_id), color); 
        wp->setStyle(GraphicsObject::Style::CURRENT_NAV);
        traj_item->addPoint(wp);
        traj_waypoints.append(wp);
    }

    traj_item->setText("AC " + QString::number(ac_id) + " GVF");
}

void GVF_trajectory::createVFieldItem(QList<QPointF> points, QList<QPointF> vpoints) 
{
    QList<Point2DLatLon> pos;
    QList<Point2DLatLon> vpos;

    for (int i=0; i<points.size(); i++) {
        pos.append(CoordinatesTransform::get()->ltp_to_wgs84(ltp_origin, points[i].x(), points[i].y()));
        vpos.append(CoordinatesTransform::get()->ltp_to_wgs84(pos[i], vpoints[i].x(), vpoints[i].y()));
    }
    
    //auto color = AircraftManager::get()->getAircraft(QString::number(ac_id))->getColor();
    auto color = Qt::red;
    vector_field = new QuiverItem(pos, vpos, QString::number(ac_id), QPen(color, 0.5));
}