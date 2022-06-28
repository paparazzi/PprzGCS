#ifndef GVF_TRAJ_ELLIPSE_H
#define GVF_TRAJ_ELLIPSE_H

#include "gvf_trajectory.h"

class GVF_traj_ellipse : public GVF_trajectory
{

public:
    GVF_traj_ellipse(QString id, Point2DLatLon origin, QList<float> param, int8_t _s, float _ke);

private:
    // TODO: Remove private parameters in a next future if I don't use them ...
    float a;
    float b;
    float alpha;
    int8_t s;
    float ke;

    void param_points();
    void vector_field();
};

#endif // GVF_TRAJ_ELLIPSE_H