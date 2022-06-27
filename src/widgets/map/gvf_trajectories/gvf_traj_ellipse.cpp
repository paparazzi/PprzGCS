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

    qDebug() << "a:" << a << " b:" << b << " alpha:" << alpha << " s:" << s << " ke:" << ke;

    QList<QPointF> vxy_mesh; 
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

        float norm = sqrt(pow(vx,2) + pow(vy,2))/12;

        vxy_mesh.append(QPointF(vx/norm, vy/norm));
    }
    
    createVFieldItem(xy_mesh, vxy_mesh);
}