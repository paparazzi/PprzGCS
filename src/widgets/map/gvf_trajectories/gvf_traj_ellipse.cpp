#include "gvf_traj_ellipse.h"

GVF_traj_ellipse::GVF_traj_ellipse(uint8_t id, QPointF pos, Point2DLatLon origin, QList<float> param, int8_t _s, float _ke) :
    GVF_trajectory(id, pos, origin)
{   
    xy_off = QPointF(param[0], param[1]);
    a = param[2];
    b = param[3];
    alpha = param[4];
    s = _s;
    ke = _ke;

    param_points();
    vector_field();
}

void GVF_traj_ellipse::param_points() { // Rotated standard ellipse parametric representation
    QList<QPointF> points;

    float dt = 0.005*2*M_PI;
    for (int i = 0; i <= 2*M_PI/dt + 1; i++) {
        float x = xy_off.x() + a*cos(alpha)*cos(dt*i) - b*sin(alpha)*sin(i*dt);
        float y = xy_off.y() + a*sin(alpha)*cos(dt*i) + b*cos(alpha)*sin(i*dt);
        points.append(QPointF(x,y));
    }

    createTrajItem(points);
}

void GVF_traj_ellipse::vector_field() { // Ellipse GVF
    QList<QPointF> vxy_mesh; 

    // TODO: El area y la cantidad de flechas se deben de ajustar al tamaño de la elipse!!
    xy_mesh = meshGrid(100000, 20, 20);

    foreach (const QPointF &point, xy_mesh) {
        float xel = (point.x() - xy_off.x())*cos(alpha) - (point.y() - xy_off.y())*sin(alpha); 
        float yel = (point.x() - xy_off.x())*sin(alpha) + (point.y() - xy_off.y())*cos(alpha); 

        float nx =  2*xel*cos(alpha)/pow(a,2) + 2*yel*sin(alpha)/pow(b,2);
        float ny = -2*xel*sin(alpha)/pow(a,2) + 2*yel*cos(alpha)/pow(b,2);

        float tx =  s*ny;
        float ty = -s*nx;

        float e = pow(xel/a,2) + pow(yel/b,2) - 1;

        float vx = tx -ke*e*nx;
        float vy = ty -ke*e*ny;

        // TODO: La norma se debe de ajustar al tamaño de la elipse!!
        float norm = sqrt(pow(vx,2) + pow(vy,2))/12;

        vxy_mesh.append(QPointF(vx/norm, vy/norm));
    }

    createVFieldItem(xy_mesh, vxy_mesh);
}