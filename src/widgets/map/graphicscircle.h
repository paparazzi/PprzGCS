#ifndef GRAPHICSCIRCLE_H
#define GRAPHICSCIRCLE_H

#include <QObject>
#include <QGraphicsEllipseItem>

enum CircleScaleState {
    CSS_IDLE,
    CSS_PRESSED,
    CSS_SCALED,
};

class GraphicsCircle : public QObject, public QGraphicsEllipseItem
{
    Q_OBJECT
public:
    explicit GraphicsCircle(double radius, QObject *parent = nullptr);
    void setRadius(double r);

signals:
    void circleScaled(qreal size);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);


public slots:

private:
    double dr;
    double radius;
    bool scalable;
    CircleScaleState scale_state;
};

#endif // GRAPHICSCIRCLE_H


