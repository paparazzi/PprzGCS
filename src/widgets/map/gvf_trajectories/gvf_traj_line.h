#ifndef GVF_TRAJ_LINE_H
#define GVF_TRAJ_LINE_H

#include "gvf_trajectory.h"

class GVF_traj_line : public GVF_trajectory
{
    Q_OBJECT
public:
    GVF_traj_line(QString id, Point2DLatLon origin, QList<float> param, int8_t _s, float _ke, QList<float> gvf_settings);

private:
    float a;
    float b;
    float course;
    int8_t s;
    double ke;

    float dx;
    float dy;

    void param_points();
    void vector_field();
};

#endif // GVF_TRAJ_LINE_H