#ifndef GVF_TRAJ_TREFOIL_H
#define GVF_TRAJ_TREFOIL_H

#include "gvf_trajectory.h"

class GVF_traj_trefoil : public GVF_trajectory
{
    Q_OBJECT
public:
    explicit GVF_traj_trefoil(QString id, QList<float> param, int8_t _s, QList<float> phi, float _wb, QVector<int> *gvf_settings);

protected:
    virtual void genTraj() override;
    virtual void genVField() override;

private:
    void set_param(QList<float> param, int8_t _s, QList<float> _phi, float _wb); // GVF PARAMETRIC
    QPointF traj_point(float t);

    float w1;
    float w2;
    float ratio;
    float r;
    float alpha;

    int8_t s;

    float phi_x;
    float phi_y;

    float wb;

    float gcd(int a, int b);
};

#endif // GVF_TRAJ_TREFOIL_H