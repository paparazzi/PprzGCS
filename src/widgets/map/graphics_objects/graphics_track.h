#ifndef GRAPHICS_TRACK_H
#define GRAPHICS_TRACK_H

#include <QObject>
#include <QGraphicsItem>
#include "graphics_object.h"

class GraphicsTrack : public GraphicsObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    explicit GraphicsTrack(PprzPalette palette, QObject *parent = nullptr);
    explicit GraphicsTrack(PprzPalette palette, QPolygonF points, QObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void addPoint(QPointF);
    void setPoints(QPolygonF pts);

protected:
    virtual void changeFocus() override;

signals:

public slots:

private:
    QPolygonF points;

    int current_color;
};

#endif // GRAPHICS_TRACK_H
