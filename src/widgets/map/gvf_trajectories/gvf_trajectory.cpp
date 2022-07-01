#include "gvf_trajectory.h"
#include "dispatcher_ui.h"

GVF_trajectory::GVF_trajectory(QString id, Point2DLatLon origin, QList<float> gvf_settings)
{
    ac_id = id;
    ltp_origin = origin;
    traj_item_vis = (int)gvf_settings[0];
    vector_field_vis = (int)gvf_settings[1];

    // If you're alive, please update your visibility when gvf_viewer request it 
    connect(DispatcherUi::get(), &DispatcherUi::gvf_settingUpdated, this,
        [=](QString sender, bool traj_vis, bool field_vis) {
            if(sender == ac_id) {
                traj_item_vis = traj_vis;
                vector_field_vis = field_vis;
                setVFiledVis(vector_field_vis);
            }
        });  
}

PathItem* GVF_trajectory::getTraj() {
    return traj_item;
}

QuiverItem* GVF_trajectory::getVField() {
    return vector_field;
}

void GVF_trajectory::setVFiledVis(bool vis) {
    vector_field_vis = vis;
    vector_field->setVisible(vector_field_vis);

}

// We have to disconnect from dispatcher and delete previous waypoints 
// if we don't want to cause memory dump or memory overflow!!
void GVF_trajectory::purge_trajectory() {
    disconnect(DispatcherUi::get(), &DispatcherUi::gvf_settingUpdated, this, 0); 

    foreach (WaypointItem* wp, traj_waypoints) {
        assert(wp != nullptr);
        delete wp;
    }
}

// Create graphics object 
void GVF_trajectory::createTrajItem(QList<QPointF> points) // TODO
{
    auto color = Qt::green;

    traj_item = new PathItem(ac_id, color);
    //traj_item->setZValues(z, z);

    for(auto point: points) {
        auto pos = CoordinatesTransform::get()->relative_utm_to_wgs84(ltp_origin, point.x(), point.y());
        auto wp =  new WaypointItem(pos, ac_id, color); 
        traj_item->addPoint(wp);
        traj_waypoints.append(wp);
    }

    traj_item->setText("AC " + ac_id + " GVF");
    //traj_item->setVisible(traj_item_vis); (TODO)
}

void GVF_trajectory::createVFieldItem(QList<QPointF> points, QList<QPointF> vpoints, float bound_area, float ref_area) 
{   
    QList<Point2DLatLon> pos;
    QList<Point2DLatLon> vpos;

    // Arrows scaling based on the trajectory bounding area
    float scale = sqrt(ref_area/bound_area); 

    for (int i=0; i<points.size(); i++) {
        float vx = vpoints[i].x();
        float vy = vpoints[i].y();

        float renorm = sqrt(pow(vx,2) + pow(vy,2))*scale;
        vx = vx/renorm;
        vy = vy/renorm;

        pos.append(CoordinatesTransform::get()->relative_utm_to_wgs84(ltp_origin, points[i].x(), points[i].y()));
        vpos.append(CoordinatesTransform::get()->ltp_to_wgs84(pos[i], vx, vy));
    }
    
    auto color = Qt::red;
    vector_field = new QuiverItem(pos, vpos, ac_id, color, 0.5);
    vector_field->setVisible(vector_field_vis);
}



/////////////// UTILITY FUNCTIONS ///////////////
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