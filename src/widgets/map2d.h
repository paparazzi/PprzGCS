#ifndef MAP2D_H
#define MAP2D_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QGraphicsTextItem>
#include "tileprovider.h"

class Map2D : public QGraphicsView
{
    Q_OBJECT
public:
    explicit Map2D(QWidget *parent = nullptr);
    void setPos(Point2DLatLon latLon);

signals:

public slots:


protected:
    virtual void wheelEvent(QWheelEvent* event);
    virtual void mouseMoveEvent(QMouseEvent *event);

private slots:
    void acChanged(int);
    void handleTile(TileItem*, Point2DTile);


private:

    void updateTiles();
    QGraphicsScene* scene;

    QGraphicsTextItem* current_ac;

    TileProvider tileProvider;

};

#endif // MAP2D_H
