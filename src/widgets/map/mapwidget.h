#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QElapsedTimer>
#include <QGraphicsItem>
#include <QList>
#include <QPointer>
#include <QTimer>
#include <QDialog>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include "map2d.h"
#include "tileprovider.h"
#include "maplayercontrol.h"
#include "imagebutton.h"
#include <QCursor>
#include "configurable.h"
#include "lock_button.h"
#include "papget.h"
#include "windindicator.h"

#include "gvf_trajectory.h"
#include "gvf_viewer.h"

#include "grid_viewer.h"
#include "grid_item.h"
#include "circle_item.h"


class ACItemManager;
class ItemEditStateMachine;
class MapItem;
class LayerCombo;
class PanelTabs;
class PathItem;

enum InteractionState {
    PMIS_FLIGHT_PLAN_EDIT,
    PMIS_FROZEN,
    PMIS_OTHER,
};

enum PanState {
    PAN_IDLE,
    PAN_PRESSED,
    PAN_MOVE
};

class MapWidget : public Map2D, public Configurable
{
    Q_OBJECT
    Q_PROPERTY(int ac_arrow_size MEMBER _ac_arrow_size WRITE setAcArrowSize)
    Q_PROPERTY(bool show_hidden_waypoints READ hiddenWaypointsShown WRITE showHiddenWaypoints)
    Q_PROPERTY(bool show_crash_prediction READ crashPredictionShown WRITE showCrashPrediction)
public:
    explicit MapWidget(QWidget *parent = nullptr);

    void addItem(MapItem* map_item);
    void removeItem(MapItem* item);
    virtual void setCursor(const QCursor &);
    void setPanMask(int mask) {pan_mouse_mask = mask; setMouseLoadTileMask(mask);}
    void itemsForbidHighlight(bool fh);
    void itemsEditable(bool ed);
    void updateHighlights(QString ac_id);

    // panels on the rail (see PanelManager)
    void addRailPanel(QString id, QWidget* widget, QString title, QString icon, int index = -1);
    /// position of a panel on the rail (-1: not on it)
    int railIndexOf(QWidget* widget);
    /// where a panel dropped at global_pos goes on the rail, and the line showing it (global)
    int railInsertAt(QPoint global_pos, QRect* line = nullptr);
    /// the size set by the user with the grip of the card (invalid: as big as the panel asks)
    void setRailCardSize(QWidget* widget, QSize size);
    /// a card moved by the user over the map (top left, in the map; invalid: next to the rail)
    void setRailCardPos(QWidget* widget, QPoint pos);
    void removeRailPanel(QWidget* widget);
    PanelTabs* bottomTabs() {return bottom_tabs;}
    MapScene* scene() {return _scene;}
    void centerLatLon(Point2DLatLon latLon) override;
    void setZoom(double z) override;

    void configure(QDomElement) override;

    void rotateMap(double rot);
    void setAcArrowSize(int s);
    void showHiddenWaypoints(bool state);
    bool hiddenWaypointsShown() {return show_hidden_wp_action->isChecked();}
    bool crashPredictionShown() {return show_crash_prediction_action->isChecked();}
    void showCrashPrediction(bool state);

signals:
    void mouseMoved(QPointF scenePos);
    void itemAdded(MapItem* map_item);
    void itemRemoved(MapItem* item);
    void railCardResized(QWidget* widget, QSize size);   // by the user (invalid size: reset)
    void railCardMoved(QWidget* widget, QPoint pos);     // by the user (invalid: docked)

protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent* event) override;
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
    virtual void dragMoveEvent(QDragMoveEvent *event) override;
    virtual bool viewportEvent(QEvent *event) override;

private slots:
    void changeCurrentAC(QString id);
    void updateAircraftItem(pprzlink::Message msg);
    void onWaypointChanged(Waypoint* wp, QString ac_id);
    void onWaypointAdded(Waypoint* wp, QString ac_id);
    void updateTarget(pprzlink::Message msg);
    void updateNavShape(pprzlink::Message msg);
    void onMoveWaypointUi(Waypoint* wp, QString ac_id);
    void onShape(QString sender, pprzlink::Message msg);
    void clearShapes();
    void clearDcShots();
    void onIntruder(QString sender, pprzlink::Message msg);
    void onDcShot(QString sender, pprzlink::Message msg);
    void onROTORCRAFT_FP(QString sender, pprzlink::Message msg);
    void onGCSPos(pprzlink::Message msg);
    void onGVF(QString sender, pprzlink::Message msg);
    void onSLAM(QString sender, pprzlink::Message msg);
    void onGridInit(QString sender, pprzlink::Message msg);
    void onGridChanges(QString sender, pprzlink::Message msg);


private:

    enum WidgetContainer {
        WIDGETS_LEFT,
        WIDGETS_RIGHT,
    };

    void handleNewAC(QString ac_id);
    void removeAC(QString ac_id);
    LayerCombo* makeLayerCombo();
    void addWidget(QWidget* w, LockButton* button, WidgetContainer side, int index = -1);
    void setEditorMode();
    void registerWaypoint(WaypointItem* waypoint);
    void updateGraphics(uint32_t update_events);

    QMap<QString, ACItemManager*> ac_items_managers;
    InteractionState interaction_state;
    QMap<WaypointItem*, QList<WaypointItem*>> wp_followers;   // waypoints at the same place moved together
    void chooseStackedWaypoint(WaypointItem* wpi, QPointF scene_pos);
    QList<WaypointItem*> waypointsAt(QPointF scene_pos);
    QList<WaypointItem*> hover_marked;     // marked as linked to the waypoint or name under the mouse
    bool hover_dimmed = false;             // the other names faded (option)
    void clearWaypointHover();
    void updateAircraftInfo(QString ac_id, pprzlink::Message msg);
    void updateWaypointHover(QPoint view_pos, bool inside);
    QPointer<WaypointItem> hover_target;   // the waypoint (or name) under the mouse
    bool hover_target_on_name = false;
    void markWaypointHover(WaypointItem* wpi, bool on_name);
    QTimer* hover_timer;                   // the marking after a short delay
    QPointer<WaypointItem> hover_pending;
    bool hover_pending_on_name = false;
    QMap<WaypointItem*, QPointer<QDialog>> waypoint_editors;   // the open card of each waypoint
    int drawState;
    ItemEditStateMachine* fp_edit_sm;
    QString current_ac;

    QList<MapItem*> _items;
    QList<Papget*> papgets;

    // a waypoint move sent to the aircraft, until it confirms it: its old position (a dashed
    // diamond) and a line to the new one
    struct PendingMove {
        QGraphicsPolygonItem* marker;
        QGraphicsLineItem* line;
        Point2DLatLon from;
        QMetaObject::Connection conn;
    };
    QMap<WaypointItem*, PendingMove> pending_moves;
    void watchWaypointMove(WaypointItem* wpi);

    // a dark layer over the imagery (map brightness), below everything drawn on the map
    QGraphicsRectItem* dim_rect = nullptr;
    void setMapDim(int percent);
    /// the aircraft names moved down when they overlap
    void arrangeAircraftLabels();
    /// the waypoint names where they do not overlap other names or waypoints
    void arrangeWaypointLabels();
    QTimer* wp_labels_timer = nullptr;

    // the position from the telemetry (ROTORCRAFT_FP), newer than FLIGHT_PARAM (about 1 Hz)
    QMap<QString, QElapsedTimer> direct_position;
    void setAircraftPose(QString ac_id, Point2DLatLon pos, double heading);
    void showPendingMove(WaypointItem* wpi);
    void updatePendingMove(WaypointItem* wpi);
    void clearPendingMove(WaypointItem* wpi);

    QMap<uint8_t, MapItem*> shapes;
    QMap<QString, GVF_trajectory*> gvf_trajectories;
    QMap<QString, QVector<int>*> gvf_trajectories_config;
    QMap<QString, std::pair<MapItem*, QTime>> intruders;
    QList<WaypointItem*> dc_shots;
    QTimer* timer_intruders;
    MapItem* gcsItem;

    QHBoxLayout* horizontalLayout;

    //ButtonBand
    QVBoxLayout* buttonsLeftLayout;
    QVBoxLayout* columnLeft;
    QVBoxLayout* rightLayout;
    QVBoxLayout* columnRight;
    QVBoxLayout* buttonsRightLayout;
    WindIndicator* wind_indicator;
    QToolButton* follow_button;
    enum FollowMode {FOLLOW_OFF = 0, FOLLOW = 1, FOLLOW_HEADING = 2};
    int follow_mode = FOLLOW_OFF;
    void setFollowMode(int mode);

    QList<LockButton*> buttonsLeft;
    PanelTabs* bottom_tabs;
    class MapNotifications* notifications = nullptr;
    void watchAircraftStates(QString ac_id);
public:
    /// Show a rail panel (as if its button was pressed)
    bool openRailPanel(QWidget* widget);
private:
    QMap<QWidget*, QFrame*> rail_cards;   // panel -> its card over the map (title, close)
    void fitRailCards();
    void setFrozen(bool frozen);
    void placeFrozenBanner();
    QPushButton* frozen_banner = nullptr;
    QPushButton* silent_banner = nullptr;
protected:
    void resizeEvent(QResizeEvent *event) override;
private:
    QList<LockButton*> buttonsRight;

    QPoint lastPos;
    PanState pan_state;
    int pan_mouse_mask;

    long shape_bind_id;

    bool gvf_loaded = false;

    QMap<int, Point2DPseudoMercator> pms;

    int _ac_arrow_size;

    QMenu* mapMenu;
    QMenu* menu_clear_track;
    QAction* show_hidden_wp_action;
    QAction* show_crash_prediction_action;

    // Obstacles
    GridItem* grid_item = nullptr;
    ObstacleGridMap* obstacle_grid_map = nullptr;
    QImage slam_grid_image;
    bool slam_grid_visible = true;
    bool obstacles_visible = false;
    QList<QPair<CircleItem*, QDateTime>> slam_obstacles;
};

#endif // MAPWIDGET_H
