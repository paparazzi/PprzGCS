#ifndef GVF_TRAJ_3D_ELLIPSE_H
#define GVF_TRAJ_3D_ELLIPSE_H

#include "gvf_trajectory.h"

class GVF_traj_3D_ellipse : public GVF_trajectory
{
    Q_OBJECT
public:
    explicit GVF_traj_3D_ellipse(QString id, QList<float> param, int8_t _s, QList<float> phi, float _wb, QVector<int> *gvf_settings);

protected:
    virtual void genTraj() override;
    virtual void genVField() override;

private:
    void set_param(QList<float> param, int8_t _s, QList<float> phi, float _wb); // GVF PARAMETRIC

    float r;
    float zl;
    float zh;
    float alpha;

    int8_t s;

    QVector3D phi;
};

#endif // GVF_TRAJ_3D_ELLIPSE_H