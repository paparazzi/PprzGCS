#ifndef MAP2D_H
#define MAP2D_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QGraphicsTextItem>
#include "osmtileprovider.h"

class Map2D : public QGraphicsView
{
    Q_OBJECT
public:
    explicit Map2D(QWidget *parent = nullptr);

signals:

public slots:


protected:
    virtual void wheelEvent(QWheelEvent* event);
    virtual void mouseMoveEvent(QMouseEvent *event);

private slots:
    void acChanged(int);
    void handleTile(TileItem*, TileCoorI);

private:
    QGraphicsScene* scene;
    int zoomLevel;

    QGraphicsTextItem* current_ac;

    OSMTileProvider tileProvider;


    ///
    /// \brief latLon0: GPQ coordinates of the (0,0) pos.
    ///
    double lat0, lon0;
    QPointF pos0;
};

#endif // MAP2D_H
