#include "gvf_traj_3D_ellipse.h"

GVF_traj_3D_ellipse::GVF_traj_3D_ellipse(QString id, QList<float> param, int8_t _s, QList<float> phi, float _wb, QVector<int> *gvf_settings) :
    GVF_trajectory(id, gvf_settings)
{   
    set_param(param, _s, phi, _wb);
    generate_trajectory();
    
}

void GVF_traj_3D_ellipse::set_param(QList<float> param, int8_t _s, QList<float> phi, float _wb) {

}

// 3D ellipse trajectory (parametric representation)
void GVF_traj_3D_ellipse::genTraj() { 
    QList<QPointF> points;

    createTrajItem(points);
}

// 3D ellipse GVF
void GVF_traj_3D_ellipse::genVField() { 

}