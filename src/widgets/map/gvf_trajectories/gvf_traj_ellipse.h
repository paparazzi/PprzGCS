#ifndef GVF_TRAJ_ELLIPSE_H
#define GVF_TRAJ_ELLIPSE_H

#include "gvf_trajectory.h"

class GVF_traj_ellipse : public GVF_trajectory
{
    Q_OBJECT
public:
    explicit GVF_traj_ellipse(QString id, Point2DLatLon origin, QList<float> param, int8_t _s, float _ke, QList<float> gvf_settings);

    virtual void set_param(QList<float> param, int8_t _s, float _ke) override;

protected:
    virtual void param_points() override;
    virtual void vector_field() override;

private:
    float a;
    float b;
    float alpha;

    int8_t s;
    float ke;
};

#endif // GVF_TRAJ_ELLIPSE_H