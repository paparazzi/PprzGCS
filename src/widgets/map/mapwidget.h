#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QGraphicsItem>
#include <QList>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "map2d.h"
#include "tileprovider.h"
#include "maplayercontrol.h"
#include "imagebutton.h"
#include <QCursor>
#include "configurable.h"
#include "lock_button.h"
#include "papget.h"

class MapItem;

enum PanState {
    PAN_IDLE,
    PAN_PRESSED,
    PAN_MOVE
};

class MapWidget : public Map2D, public Configurable
{
    Q_OBJECT
public:
    explicit MapWidget(QWidget *parent = nullptr);

    void addItem(MapItem* map_item);
    void removeItem(MapItem* item);
    MapLayerControl* makeLayerControl(QString name, bool initialState, int z);
    virtual void setCursor(const QCursor &);
    void setPanMask(int mask) {pan_mouse_mask = mask; setMouseLoadTileMask(mask);}
    void itemsForbidHighlight(bool fh);
    void itemsEditable(bool ed);
    void updateHighlights(QString ac_id);
    MapScene* scene() {return _scene;}
    void centerLatLon(Point2DLatLon latLon) override;
    void setZoom(double z) override;

    void configure(QDomElement);

signals:
    void mouseMoved(QPointF scenePos);
    void itemAdded(MapItem* map_item);
    void itemRemoved(MapItem* item);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);

private:

    enum WidgetContainer {
        WIDGETS_LEFT,
        WIDGETS_RIGHT,
    };

    void handleNewAC(QString ac_id);
    void addLayersWidget();
    void addWidget(QWidget* w, LockButton* button, WidgetContainer side);

    QList<MapItem*> _items;
    QList<Papget*> papgets;

    QHBoxLayout* horizontalLayout;

    //ButtonBand
    QVBoxLayout* buttonsLeftLayout;
    QVBoxLayout* columnLeft;
    QVBoxLayout* columnRight;
    QVBoxLayout* buttonsRightLayout;

    QList<LockButton*> buttonsLeft;
    QList<LockButton*> buttonsRight;

    QPoint lastPos;
    PanState pan_state;
    int pan_mouse_mask;

};

#endif // MAPWIDGET_H
