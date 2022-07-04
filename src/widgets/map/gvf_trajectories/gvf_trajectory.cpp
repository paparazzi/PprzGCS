#include "gvf_trajectory.h"
#include "dispatcher_ui.h"

GVF_trajectory::GVF_trajectory(QString id, Point2DLatLon origin, QList<float> gvf_settings)
{
    ac_id = id;
    ltp_origin = origin;
    traj_item_vis = (int)gvf_settings[0];
    field_item_vis = (int)gvf_settings[1];
    //field_area = gvf_settings[2]; // TODO: Incluir dentro de la widget
    //field_xpts = (int)gvf_settings[3];
    //field_ypts = (int)gvf_settings[4];  

    // If you're alive, please update your settings and update when gvf_viewer request it 
    connect(DispatcherUi::get(), &DispatcherUi::gvf_settingUpdated, this,
        [=](QString sender, bool traj_vis, bool field_vis) {
            if(sender == ac_id) {
                traj_item_vis = traj_vis;
                field_item_vis = field_vis;
                setVFiledVis(field_item_vis);
            }
        });

    // TODO: Cuando  
}

PathItem* GVF_trajectory::getTraj() {
    return traj_item;
}

QuiverItem* GVF_trajectory::getVField() {
    return field_item;
}

void GVF_trajectory::setVFiledVis(bool vis) {
    field_item_vis = vis;
    field_item->setVisible(field_item_vis);

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

// Regenerate the trajectory points and the vectory field
void GVF_trajectory::update_trajectory() {
    genTraj();
    genVField();
}

/////////////// PRIVATE FUNCTIONS ///////////////
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
    //traj_item->setVisible(traj_item_vis); //(TODO)
}

void GVF_trajectory::createVFieldItem(QList<QPointF> points, QList<QPointF> vpoints, float bound_area, float ref_area) 
{   
    auto color = Qt::red;
    field_item = new QuiverItem(ac_id, color, 0.5, this);

    // Arrows scaling based on the trajectory bounding area
    float scale = sqrt(ref_area/bound_area); 

    for (int i=0; i<points.size(); i++) {
        float vx = vpoints[i].x();
        float vy = vpoints[i].y();

        float renorm = sqrt(pow(vx,2) + pow(vy,2))*scale;
        vx = vx/renorm;
        vy = vy/renorm;

        
        auto pos_latlon  = CoordinatesTransform::get()->relative_utm_to_wgs84(ltp_origin, points[i].x(), points[i].y());
        auto vpos_latlon = CoordinatesTransform::get()->relative_utm_to_wgs84(pos_latlon, vx, vy);
        // AQUÍ ESTÁ EL PROBLEMA!!! (en meter el pos_latlon dentro de ltp_to_wgs84)

        field_item->addQuiver(pos_latlon, vpos_latlon);  
    }

    field_item->setVisible(field_item_vis);
}

// Create the XY mesh to draw the vectory field
QList<QPointF> GVF_trajectory::meshGrid(float area, int xpoints_num, int ypoints_num) //TODO: No va a necesitar parámetros, serán variables protegidas
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

// Get AC position in LTP
QPointF GVF_trajectory::getACpos() {
    auto latlon = Point2DLatLon(0,0);

    auto ac = pprzApp()->toolbox()->aircraftManager()->getAircraft(ac_id);
    auto flight_param_msg = ac->getStatus()->getMessage("FLIGHT_PARAM");

    if(flight_param_msg) {
        double lat,lon;
        flight_param_msg->getField("lat" ,lat);
        flight_param_msg->getField("long",lon);
        latlon = Point2DLatLon(lat,lon);
    } else {
        qDebug() << "GVF_TRAJECTORY: Can't read FLIGHT_PARAM of AC" << ac_id << ".";
        return QPointF(0,0);
    }

    QPointF ac_xy;
    CoordinatesTransform::get()->wgs84_to_ltp(ltp_origin, latlon, ac_xy.rx(), ac_xy.ry());

    return ac_xy;
}   