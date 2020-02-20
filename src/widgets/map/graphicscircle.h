#ifndef GRAPHICSCIRCLE_H
#define GRAPHICSCIRCLE_H

#include <QObject>
#include <QGraphicsEllipseItem>
#include <QPen>
#include "graphicsobject.h"

enum CircleScaleState {
    CSS_IDLE,
    CSS_PRESSED,
    CSS_SCALED,
};

class GraphicsCircle : public GraphicsObject, public QGraphicsEllipseItem
{
    Q_OBJECT
public:
    explicit GraphicsCircle(double radius, QPen pen_idle, QObject *parent = nullptr);
    void setRadius(double r);
    void setScalable(bool sc) {scalable = sc;}
    void setColors(QColor colPressed, QColor colScaling, QColor colUnfocused);

signals:
    void circleScaled(qreal size);
    void circleScaleFinished();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void changeFocus();


public slots:

private:
    double dr;
    double radius;
    bool scalable;
    CircleScaleState scale_state;

    QPen pen_idle;
    QPen pen_pressed;
    QPen pen_scaling;
    QPen pen_unfocused;
};

#endif // GRAPHICSCIRCLE_H


