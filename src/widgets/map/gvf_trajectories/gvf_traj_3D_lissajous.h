#ifndef GVF_TRAJ_3D_LISSAJOUS_H
#define GVF_TRAJ_3D_LISSAJOUS_H

#include "gvf_trajectory.h"

class GVF_traj_3D_lissajous : public GVF_trajectory
{
    Q_OBJECT
public:
    explicit GVF_traj_3D_lissajous(QString id, Point2DLatLon origin, QList<float> param, int8_t _s, QList<float> phi, QVector<int> *gvf_settings);

protected:
    virtual void genTraj() override;
    virtual void genVField() override;

private:
    void set_param(QList<float> param, int8_t _s, QList<float> phi); // GVF PARAMETRIC

    QVector3D off;
    QVector3D c;
    QVector3D w;
    QVector3D d;
    float alpha;

    int8_t s;

    QVector3D phi;
};

#endif // GVF_TRAJ_3D_LISSAJOUS_H