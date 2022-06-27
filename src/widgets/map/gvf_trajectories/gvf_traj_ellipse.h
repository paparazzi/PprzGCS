#ifndef GVF_TRAJ_ELLIPSE_H
#define GVF_TRAJ_ELLIPSE_H

#include "gvf_trajectory.h"

class GVF_traj_ellipse : public GVF_trajectory
{

public:
    GVF_traj_ellipse(uint8_t id, QPointF pos, Point2DLatLon origin, QList<float> param, int8_t _s, float _ke);

private:
    float a;
    float b;
    float alpha;
    int8_t s;
    float ke;
};

#endif // GVF_TRAJ_ELLIPSE_H