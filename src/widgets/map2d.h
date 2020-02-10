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

class Map2D : public QGraphicsView
{
    Q_OBJECT
public:
    explicit Map2D(QWidget *parent = nullptr);
    void setPos(Point2DLatLon latLon, double cx=0, double cy=0);

signals:

public slots:


protected:
    virtual void wheelEvent(QWheelEvent* event);
    virtual void mouseMoveEvent(QMouseEvent *event);

private slots:
    void acChanged(int);
    void handleTile(TileItem*, TileItem*);


private:

    static constexpr double NUMERIC_ZOOM_FACTOR = 0.3;
    std::map<QString, std::unique_ptr<TileProviderConfig>> loadConfig(QString filename);
    void updateTiles();
    QGraphicsScene* scene;

    double numericZoom;
    double zoom;
    int tileSize;

    std::map<QString, std::unique_ptr<TileProviderConfig>> sourceConfigs;
    TileProvider* tileProvider;

};

#endif // MAP2D_H
