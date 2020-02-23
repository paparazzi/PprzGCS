#ifndef GRAPHICSCIRCLE_H
#define GRAPHICSCIRCLE_H

#include <QObject>
#include <QGraphicsItem>
#include <QFont>
#include "graphicsobject.h"

enum CircleScaleState {
    CSS_IDLE,
    CSS_PRESSED,
    CSS_SCALED,
};

class GraphicsCircle : public GraphicsObject, public QGraphicsItem
{
    Q_OBJECT
public:
    explicit GraphicsCircle(double radius, QColor color, int stroke, QObject *parent = nullptr);
    void setRadius(double r);
    void setColors(QColor colPressed, QColor colScaling, QColor colUnfocused);
    void displayRadius(bool dpr) {display_radius = dpr; update();}
    void setTextPos(QPointF pos) {textPos = pos;}
    void setText(QString t) {text = t;}

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;

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
    QPointF textPos;
    double radius;
    CircleScaleState scale_state;

    QColor* current_color;

    QColor color_idle;
    QColor color_pressed;
    QColor color_scaling;
    QColor color_unfocused;

    int base_stroke;
    int stroke;
    QPainterPath path_draw;
    QPainterPath path_shape;
    QString text;

    bool display_radius;
    QRectF last_bounding_rect;
};

#endif // GRAPHICSCIRCLE_H


