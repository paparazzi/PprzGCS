#include "gvf_traj_sin.h"

GVF_traj_sin::GVF_traj_sin(QString id, Point2DLatLon origin, QList<float> param, int8_t _s, float _ke, QList<float> gvf_settings) :
    GVF_trajectory(id, origin, gvf_settings)
{   
    // INIT
    set_param(param, _s, _ke);
    update_trajectory();
}

// Get all the necessary parameters to construct the sin trajectory
void GVF_traj_sin::set_param(QList<float> param, int8_t _s, float _ke) {
    if (param.size()>6) {
        QPointF xy_wp1;
        
        auto ac = pprzApp()->toolbox()->aircraftManager()->getAircraft(ac_id);
        Waypoint::WpFrame frame = ac->getFlightPlan()->getFrame();
        ac->getFlightPlan()->getWaypoint((uint8_t)param[6])->getRelative(frame, xy_wp1.rx(), xy_wp1.ry());

        a = xy_wp1.x();
        b = xy_wp1.y();

        if(param.size()>7) { // gvf_sin_wp1_wp2()
            QPointF xy_wp2;
            
            ac->getFlightPlan()->getWaypoint((uint8_t)param[7])->getRelative(frame, xy_wp2.rx(), xy_wp2.ry());

            dx = xy_wp2.x() - xy_wp1.x();
            dy = xy_wp2.y() - xy_wp1.y();

            course = atan2f(dy, dx);

        } else { // gvf_sin_wp_alpha()
            dx = 200;
            dy = 200;

            course = param[2];
            qDebug() << "gvf_sin_wp_alpha()";
        }

    } else {// gvf_sin_XY_alpha()
        a = param[0];
        b = param[1];

        dx = 200;
        dy = 200;

        course = param[2];
        qDebug() << "gvf_sin_XY_alpha()";
    }

    w   = param[3];
    off = param[4];
    A   = param[5];
    s = _s;
    ke = _ke;
    
    xy_off = QPointF(a,b); //TODO: Vectory field should follow the AC in "non limited" trajectories
}

// Sin parametric representation
void  GVF_traj_sin::param_points() { 
    QList<QPointF> points;

    float dr = sqrt(pow(dx,2) + pow(dy,2));

    int dt = 1;
    for (float xtr = -2*dr; xtr <= 2*dr + dt/2; xtr+=dt) {
        float ytr = A*sin(w*xtr + off);

        float x = - xtr*sin(course) + ytr*cos(course) + a;
        float y =   xtr*cos(course) + ytr*sin(course) + b;
        points.append(QPointF(x,y));
    }

    createTrajItem(points);
}

// Sin GVF
void GVF_traj_sin::vector_field() {
    QList<QPointF> vxy_mesh; 
    
    float bound_area = pow(dx,2) + pow(dy,2); // to scale the arrows
    xy_mesh = meshGrid(bound_area*2, 25, 25);

    foreach (const QPointF &point, xy_mesh) {
        double xs =  (point.x() - a)*sin(course) - (point.y() - b)*cos(course);
        double ys = -(point.x() - a)*cos(course) - (point.y() - b)*sin(course);

        double ang = w*xs + off;

        double nx = -cos(course) - A*w*cos(ang)*sin(course);
        double ny = -sin(course) + A*w*cos(ang)*cos(course);

        double tx =  s*ny;
        double ty = -s*nx;

        double e = ys - A*sin(ang);

        double vx = tx - (1e-2*ke)*e*nx;
        double vy = ty - (1e-2*ke)*e*ny;

        double norm = sqrt(pow(vx,2) + pow(vy,2));

        vxy_mesh.append(QPointF(vx/norm, vy/norm));
    }

    createVFieldItem(xy_mesh, vxy_mesh, bound_area, 500);
}