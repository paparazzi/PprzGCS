#ifndef GVF_TRAJ_LINE_H
#define GVF_TRAJ_LINE_H

#include "gvf_trajectory.h"

class GVF_traj_line : public GVF_trajectory
{
    Q_OBJECT
public:
    explicit GVF_traj_line(QString id, Point2DLatLon origin, QList<float> param, int8_t _s, float _ke, QList<float> gvf_settings);

    virtual void set_param(QList<float> param, int8_t _s, float _ke) override;

protected:
    virtual void genTraj() override;
    virtual void genVField() override;

private:
    float a;
    float b;
    float course;
    
    int8_t s;
    double ke;

    float dx;
    float dy;
};

#endif // GVF_TRAJ_LINE_H