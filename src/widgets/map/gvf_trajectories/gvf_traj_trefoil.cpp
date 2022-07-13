#include "gvf_traj_trefoil.h"

GVF_traj_trefoil::GVF_traj_trefoil(QString id, Point2DLatLon origin, QList<float> param, int8_t _s, QList<float> phi, QVector<int> *gvf_settings) :
    GVF_trajectory(id, origin, gvf_settings)
{   
    set_param(param, _s, phi);
    update_trajectory();
}

// Get all the necessary parameters to construct the ellipse trajectory
void GVF_traj_trefoil::set_param(QList<float> param, int8_t _s, QList<float> phi) {

}

// 2D trefoil trajectory (parametric representation)
void GVF_traj_trefoil::genTraj() { 
    QList<QPointF> points;

    createTrajItem(points);
}

// 2D trefoil GVF 
void GVF_traj_trefoil::genVField() { 

}