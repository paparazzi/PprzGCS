#ifndef PPRZMAP_H
#define PPRZMAP_H

#include <QWidget>
#include <QKeyEvent>
#include "mapitem.h"
#include "fpeditstatemachine.h"

namespace Ui {
class PprzMap;
}

enum InteractionState {
    PMIS_FLIGHT_PLAN_EDIT,
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
    Ui::PprzMap *ui;
    int drawState;
    InteractionState interaction_state;
    FpEditStateMachine* fp_edit_sm;

    QList<MapItem*> items;

};

#endif // PPRZMAP_H
