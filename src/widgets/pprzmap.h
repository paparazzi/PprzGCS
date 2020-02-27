#ifndef PPRZMAP_H
#define PPRZMAP_H

#include <QWidget>
#include <QKeyEvent>
#include "map_item.h"
#include "item_edit_state_machine.h"
#include "aircraft_item.h"
#include <pprzlink/Message.h>

namespace Ui {
class PprzMap;
}

enum InteractionState {
    PMIS_FLIGHT_PLAN_EDIT,
    PMIS_FROZEN,
    PMIS_OTHER,
};

class PprzMap : public QWidget
{
    Q_OBJECT

public:
    explicit PprzMap(QWidget *parent = nullptr);
    ~PprzMap();
    void registerWaypoint(WaypointItem* waypoint);

protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

    void scenePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void sceneMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void sceneReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);


protected slots:

private:
    void updateAircraftItem(pprzlink::Message msg);
    void saveItem(MapItem* item);
    void setEditorMode();
    Ui::PprzMap *ui;
    int drawState;
    InteractionState interaction_state;
    ItemEditStateMachine* fp_edit_sm;

    std::map<QString, AircraftItem*> aircraft_items;

    QString current_ac;


};

#endif // PPRZMAP_H
