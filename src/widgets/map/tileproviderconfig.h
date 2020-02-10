#ifndef TILEPROVIDERCONFIG_H
#define TILEPROVIDERCONFIG_H

#include <QString>
#include <memory>

class TileProviderConfig
{
public:
    class builder;

    TileProviderConfig(QString name, QString dir, QString addr,
                       int posZoom, int posX, int posY,
                       int zoomMin, int zoomMax, int tileSize, QString format):
        name(name), dir(dir), addr(addr),
        posZoom(posZoom), posX(posX), posY(posY),
        zoomMin(zoomMin), zoomMax(zoomMax), tileSize(tileSize), format(format)
    {
    }

    void printConfig();

    const QString name;
    const QString dir;
    const QString addr;
    const int posZoom;
    const int posX;
    const int posY;
    const int zoomMin;
    const int zoomMax;
    const int tileSize;
    const QString format;
};

class TileProviderConfig::builder
{
public:
    builder& setName(QString value) { name = value; return *this;}
    builder& setDir(QString value) { dir = value; return *this;}
    builder& setAddr(QString value) { addr = value; return *this;}
    builder& setPosZoom(int value) { posZoom = value; return *this;}
    builder& setPosX(int value) { posX = value; return *this;}
    builder& setPosY(int value) { posY = value; return *this;}
    builder& setZoomMin(int value) { zoomMin = value; return *this;}
    builder& setZoomMax(int value) { zoomMax = value; return *this;}
    builder& setTileSize(int value) { tileSize = value; return *this;}
    builder& setFormat(QString value) { format = value; return *this;}

    TileProviderConfig build() const {
        return TileProviderConfig(name, dir, addr, posZoom, posX, posY, zoomMin, zoomMax, tileSize, format);
    }

    TileProviderConfig* newBuild() const {
        return new TileProviderConfig(name, dir, addr, posZoom, posX, posY, zoomMin, zoomMax, tileSize, format);
    }

    std::unique_ptr<TileProviderConfig> buildUnique() const {
        return std::unique_ptr<TileProviderConfig>(
                    new TileProviderConfig(name, dir, addr, posZoom, posX, posY, zoomMin, zoomMax, tileSize, format)
        );
    }

private:
    QString name;
    QString dir;
    QString addr;
    int posZoom;
    int posX;
    int posY;
    int zoomMin;
    int zoomMax;
    int tileSize;
    QString format;

};

#endif // TILEPROVIDERCONFIG_H
