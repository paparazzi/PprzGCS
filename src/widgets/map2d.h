#ifndef MAP2D_H
#define MAP2D_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QGraphicsTextItem>
#include "tileprovider.h"
#include "tileproviderconfig.h"
#include <QMap>
#include <math.h>
#include <QResizeEvent>

class Map2D : public QGraphicsView
{
    Q_OBJECT
public:
    explicit Map2D(QWidget *parent = nullptr);
    void centerLatLon(Point2DLatLon latLon);

signals:

public slots:


protected:
    virtual void wheelEvent(QWheelEvent* event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void resizeEvent(QResizeEvent *event);

private slots:
    void acChanged(int);
    void handleTile(TileItem*, TileItem*);


private:

    Point2DTile tilePoint(QPointF scenePos, int zoom);
    QPointF scenePoint(Point2DTile tilePoint);
    QPointF scenePoint(Point2DLatLon latlon, int zoomLvl);

    int zoomLevel() {return static_cast<int>(ceil(zoom));}

    static constexpr double NUMERIC_ZOOM_FACTOR = 0.3;
    std::map<QString, std::unique_ptr<TileProviderConfig>> loadConfig(QString filename);
    void updateTiles();
    QGraphicsScene* scene;

    double numericZoom;
    double zoom;
    int tileSize;
    double minZoom;
    double maxZoom;

    std::map<QString, std::unique_ptr<TileProviderConfig>> sourceConfigs;
    TileProvider* tileProvider;

};

#endif // MAP2D_H
