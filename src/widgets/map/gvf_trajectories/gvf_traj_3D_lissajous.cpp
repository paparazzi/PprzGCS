#include "gvf_traj_3D_lissajous.h"

GVF_traj_3D_lissajous::GVF_traj_3D_lissajous(QString id, QList<float> param, int8_t _s, QList<float> phi, float _wb, QVector<int> *gvf_settings) :
    GVF_trajectory(id, gvf_settings)
{   
    set_param(param, _s, phi, _wb);
    generate_trajectory();
    
}

void GVF_traj_3D_lissajous::set_param(QList<float> param, int8_t _s, QList<float> phi, float _wb) {

}

// 3D lissajous trajectory (parametric representation)
void GVF_traj_3D_lissajous::genTraj() { 
    QList<QPointF> points;

    createTrajItem(points);
}

// 3D lissajous GVF
void GVF_traj_3D_lissajous::genVField() { 

}