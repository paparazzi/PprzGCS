#ifndef MAP2D_H
#define MAP2D_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include "tileprovider.h"
#include "tileproviderconfig.h"
#include <QMap>
#include <math.h>
#include <QResizeEvent>
#include "mapscene.h"
#include "maputils.h"

class Map2D : public QGraphicsView
{
    Q_OBJECT
public:
    explicit Map2D(QWidget *parent = nullptr);
    virtual void centerLatLon(Point2DLatLon latLon);
    void toggleTileProvider(QString providerName, bool enable, int zValue = 0, qreal opacity = 1);
    std::map<QString, TileProviderConfig*>* tileProviders() {return &sourceConfigs;}
    QList<QString> tileProvidersNames();
    void updateTiles();
    void getViewPoints(Point2DLatLon& nw, Point2DLatLon& sw);
    double zoom() {return _zoom;}
//    int zoomLevel() {return static_cast<int>(ceil(_zoom));}
    virtual void setZoom(double z);
    void zoomCentered(double z, QPoint eventPos);
    int tileSize() {return tile_size;}

    void setLayerOpacity(QString providerName, qreal opacity);
    void setLayerZ(QString providerName, int z);
    double scaleFactor() {return pow(2, _zoom - zoomLevel(_zoom));}
    void setMouseLoadTileMask(int mask) {mouse_load_tiles_mask = mask;}

    //Point2DLatLon latlonPoint(QPointF scenePos, int zoom);

    ///
    /// \brief setTilesPath set directory under which tiles are stored for all tiles providers
    /// \param path
    ///
    void setTilesPath(QString path);

    ///
    /// \brief setTilesPath set directory under which tiles are stored for a specific tiles providers
    /// \param path
    /// \param providerName
    /// \return
    ///
    bool setTilesPath(QString path, QString providerName);

signals:

public slots:


protected:
    virtual void wheelEvent(QWheelEvent* event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);

    Point2DLatLon latlonFromView(QPoint viewPos, int zoom);


    //QGraphicsScene* _scene;
    MapScene* _scene;



private slots:
    void handleTile(TileItem*, TileItem*);


private:
    void loadConfig(QString filename);

    int mouse_load_tiles_mask;
    double numericZoom;
    double _zoom;
    int tile_size;
    double minZoom;
    double maxZoom;
    QString tiles_path;

    int wheelAccumulator;

    std::map<QString, TileProviderConfig*> sourceConfigs;
    std::map<QString, TileProvider*> tile_providers;

};

#endif // MAP2D_H
