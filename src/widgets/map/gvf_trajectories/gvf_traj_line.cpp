#include "gvf_traj_line.h"

GVF_traj_line::GVF_traj_line(QString id, Point2DLatLon origin, QList<float> param, int8_t _s, float _ke, QVector<int> *gvf_settings) :
    GVF_trajectory(id, origin, gvf_settings)
{   
    // INIT
    set_param(param, _s, _ke);
    update_trajectory();
}

// Get all the necessary parameters to construct the line trajectory
void GVF_traj_line::set_param(QList<float> param, int8_t _s, float _ke) {
    if (param.size()>3) { // gvf_line_wp()
        QPointF xy_wp1;
        QPointF xy_wp2; 

        auto ac = pprzApp()->toolbox()->aircraftManager()->getAircraft(ac_id);
        Waypoint::WpFrame frame = ac->getFlightPlan()->getFrame();
        ac->getFlightPlan()->getWaypoint((uint8_t)param[3])->getRelative(frame, xy_wp1.rx(), xy_wp1.ry());
        ac->getFlightPlan()->getWaypoint((uint8_t)param[4])->getRelative(frame, xy_wp2.rx(), xy_wp2.ry());

        a = xy_wp1.x();
        b = xy_wp1.y();

        dx = xy_wp2.x() - xy_wp1.x();
        dy = xy_wp2.y() - xy_wp1.y();

        course = atan2f(dx, dy);

    } else { // gvf_line_XY()
        a = param[0];
        b = param[1];

        dx = 200;
        dy = 200;

        course = param[2];
    }

    s = _s;
    ke = _ke;
    
    xy_off = getACpos();
}

// Line parametric representation
void GVF_traj_line::genTraj() { 
    QList<QPointF> points;

    float dr = sqrt(pow(dx,2) + pow(dy,2));
    
    float dt = dr/100;
    for (float t = 0; t <=  + dt/2; t+=dt) {
        float x = t*sin(course) + a;
        float y = t*cos(course) + b;
        points.append(QPointF(x,y));
    }

    createTrajItem(points);
}

// Line GVF
void GVF_traj_line::genVField() {
    QPointF xy_off;
    QList<QPointF> vxy_mesh; 
    
    float bound_area = pow(dx,2) + pow(dy,2); // to scale the arrows

    emit DispatcherUi::get()->gvf_defaultFieldSettings(ac_id, bound_area, 25, 25);
    xy_mesh = meshGrid(); //4*bound_area TODO: Fix auto scale method

    foreach (const QPointF &point, xy_mesh) {
        double nx = -cos(course);
        double ny =  sin(course);

        double tx =  s*ny;
        double ty = -s*nx;

        double e = (point.x() - a)*nx + (point.y() - b)*ny;

        double vx = tx - (1e-2*ke)*e*nx;
        double vy = ty - (1e-2*ke)*e*ny;

        double norm = sqrt(pow(vx,2) + pow(vy,2));

        vxy_mesh.append(QPointF(vx/norm, vy/norm));
    }

    createVFieldItem(xy_mesh, vxy_mesh, 500);
}