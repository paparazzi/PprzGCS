#ifndef WAYPOINTEDITOR_H
#define WAYPOINTEDITOR_H

#include <QtWidgets>
#include <memory>
#include "waypoint.h"
#include "waypoint_item.h"
#include "dispatcher_ui.h"
#include "card_dialog.h"

/**
 * Move a waypoint: a card next to the waypoint with how far and which way it moved, the
 * coordinates (in WGS84 or relative to a waypoint), the altitude with its old value and the
 * height above ground, and Move (Enter) or Cancel (Esc). Waypoints at the same place can be moved
 * along (followers).
 */
class WaypointEditor : public CardDialog
{
    Q_OBJECT
public:
    /// followers: waypoints at the same place moved with it (same position and altitude)
    explicit WaypointEditor(WaypointItem* wi, QString ac_id, QWidget *parent = nullptr,
                            QList<WaypointItem*> followers = {});

protected:
    void showEvent(QShowEvent* event) override;

private:
    void updateInfo();
    QPushButton* move_button = nullptr;

    WaypointItem* wi;
    QList<WaypointItem*> followers;
    QString ac_id;
    Point2DLatLon original_pos;
    double original_alt;
    QLabel* moved_label;
    QLabel* was_label;
    QLabel* agl_label;
};

#endif // WAYPOINTEDITOR_H
