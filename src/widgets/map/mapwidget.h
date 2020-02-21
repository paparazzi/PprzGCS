#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QGraphicsItem>
#include <QList>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include "map2d.h"
#include "tileprovider.h"
#include "maplayercontrol.h"
#include "imagebutton.h"
#include "layertab.h"
#include <QCursor>

class MapItem;

enum PanState {
    PAN_IDLE,
    PAN_PRESSED,
    PAN_MOVE
};

class MapWidget : public Map2D
{
    Q_OBJECT
public:
    explicit MapWidget(QWidget *parent = nullptr);

    void addItem(MapItem* map_item);
    void addLayerControl(QString name, bool initialState, int z);
    virtual void setCursor(const QCursor &);
    void setPanMask(int mask) {pan_mouse_mask = mask;}

signals:
    void rightClick(QMouseEvent *event);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent* event);

private:
    void setupUi();

    QList<MapItem*> _items;

    QHBoxLayout* horizontalLayout;
    LayerTab* layer_tab;
    QVBoxLayout* columnLeft;
    QVBoxLayout* columnRight;
    QSpacerItem* spacer;
    QScrollArea* leftScrollArea;
    ImageButton* layers_button;

    QPoint lastPos;
    PanState pan_state;
    int pan_mouse_mask;

};

#endif // MAPWIDGET_H
