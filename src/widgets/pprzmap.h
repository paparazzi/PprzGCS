#ifndef PPRZMAP_H
#define PPRZMAP_H

#include <QWidget>
#include <QKeyEvent>
#include "map_item.h"
#include "item_edit_state_machine.h"
#include "aircraft_item.h"
#include <pprzlink/Message.h>
#include "coordinatestransform.h"
#include "QScrollArea"
#include "layer_combo.h"
#include "maplayercontrol.h"
#include "configurable.h"

namespace Ui {
class PprzMap;
}

enum InteractionState {
    PMIS_FLIGHT_PLAN_EDIT,
    PMIS_FROZEN,
    PMIS_OTHER,
};

class PprzMap : public QWidget, public Configurable
{
    Q_OBJECT

public:
    explicit PprzMap(QWidget *parent = nullptr);
    ~PprzMap();
    void registerWaypoint(WaypointItem* waypoint);
    void setMapLayout(QLayout* layout);
    MapLayerControl* makeLayerControl(QString name, bool initialState, int z);
    void configure(QDomElement e);

protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

    void scenePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void sceneMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void sceneReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);


protected slots:
    void handleMouseMove(QPointF scenePos);

private slots:
    void updateAircraftItem(pprzlink::Message msg);
    void moveWaypoint(pprzlink::Message msg);
    void updateTarget(pprzlink::Message msg);
    void updateNavShape(pprzlink::Message msg);
    void changeCurrentAC(QString id);

private:
    void handleNewAC(QString ac_id);
    void removeAC(QString ac_id);
    void saveItem(MapItem* item);
    void setEditorMode();
    QString sexagesimalFormat(double lat, double lon);
    Ui::PprzMap *ui;
    int drawState;
    InteractionState interaction_state;
    ItemEditStateMachine* fp_edit_sm;
    QList<WaypointItem*> waypointItems;
    QList<WaypointItem*> targets;
    QList<MapItem*> current_nav_shapes;
    QMap<QString, AircraftItem*> aircraft_items;

    QString current_ac;

    //CoordinatesTransform ct_wgs84_utm;
};

#endif // PPRZMAP_H
