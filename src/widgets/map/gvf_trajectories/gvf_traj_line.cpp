#include "gvf_traj_line.h"

GVF_traj_line::GVF_traj_line(QString id, Point2DLatLon origin, QList<float> param, int8_t _s, float _ke, QList<float> gvf_settings) :
    GVF_trajectory(id, origin, gvf_settings)
{   
    // Get the two points of the line
    if (param.size()>3) { // gvf_line_wp()
        QPointF xy_wp1;
        QPointF xy_wp2; 

        auto ac = pprzApp()->toolbox()->aircraftManager()->getAircraft(id);
        Waypoint::WpFrame frame = ac->getFlightPlan()->getFrame();
        ac->getFlightPlan()->getWaypoint((uint8_t)param[3])->getRelative(frame, xy_wp1.rx(), xy_wp1.ry());
        ac->getFlightPlan()->getWaypoint((uint8_t)param[4])->getRelative(frame, xy_wp2.rx(), xy_wp2.ry());

        a = xy_wp1.x();
        b = xy_wp1.y();

        dx = xy_wp2.x() - xy_wp1.x();
        dy = xy_wp2.y() - xy_wp1.y();

        course = atan2f(dx, dy);
        xy_off = QPointF((xy_wp2.x() + xy_wp1.x())/2,(xy_wp2.y() + xy_wp1.y())/2);

    } else { // gvf_line_XY()
        a = param[0];
        b = param[1];

        dx = 200;
        dy = 200;

        course = param[2];
        xy_off = QPointF(a,b);
    }

    // Get the rest of GVF line trajectory parameters
    s = _s;
    ke = _ke;

    param_points();
    vector_field();
}

// Line parametric representation
void GVF_traj_line::param_points() { 
    QList<QPointF> points;

    int dt = 1;
    for (float t = 0; t <= sqrt(pow(dx,2) + pow(dy,2)) + dt/2; t+=dt) {
        float x = t*sin(course) + a;
        float y = t*cos(course) + b;
        points.append(QPointF(x,y));
    }

    createTrajItem(points);
}

// Line GVF
void GVF_traj_line::vector_field() {
    QPointF xy_off;
    QList<QPointF> vxy_mesh; 
    
    float bound_area = pow(dx,2) + pow(dy,2); // to scale the arrows
    xy_mesh = meshGrid(bound_area*2, 25, 25);

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

    createVFieldItem(xy_mesh, vxy_mesh, bound_area, 500);
}