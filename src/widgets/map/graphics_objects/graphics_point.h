#ifndef GRAPHICSPOINT_H
#define GRAPHICSPOINT_H

#include <QObject>
#include <QGraphicsItem>
#include <QElapsedTimer>
#include <optional>
#include "graphics_object.h"
#include <QBrush>
#include "pprzpalette.h"

enum PointMoveState {
    PMS_IDLE,
    PMS_PRESSED,
    PMS_MOVED,
    PMS_RELEASED,   // dragged, then released: finished after a short grace period (see below)
    PMS_CARRIED,    // follows the mouse without a button held, until a click places it
};

class GraphicsPoint : public GraphicsObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
    Q_PROPERTY(int size MEMBER halfSize)
public:
    explicit GraphicsPoint(int size, PprzPalette palette, QObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;
    /// a blue ring: linked to the name under the mouse
    void setHoverMark(bool h) {if(hover_mark != h) {hover_mark = h; update();}}
    /// the ground station: its heading (deg, from the north), when known
    void setHeading(std::optional<double> h) {if(heading != h) {heading = h; update();}}
    /// it follows the mouse (no button held) until a click places it (as a drag released there);
    /// a right click or cancelCarry puts it back (pointMoveCancelled)
    void startCarry();
    void cancelCarry();
    bool isCarried() {return move_state == PMS_CARRIED;}

signals:
    /// pressed (also on its name); ctrl: the Ctrl key held
    void pointPressed(QPointF scenePos, bool ctrl);
    void pointMoved(QPointF scenePos);
    void pointMoveFinished(QPointF scenePos);
    /// pressed again right after pointMoveFinished (the button was not really released): the
    /// same drag goes on, pointMoveFinished again at its end
    void pointMoveResumed();
    /// carried, then put back (right click, Esc)
    void pointMoveCancelled();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    /// the mouse taken away (QEvent::UngrabMouse)
    bool sceneEvent(QEvent* event) override;
    void mouseUngrabbed();
    virtual void changeFocus() override;
    /// the mouse on its name (installSceneEventFilter): handled as on the point
    bool sceneEventFilter(QGraphicsItem* watched, QEvent* event) override;



public slots:

private:
    /// the end of the grace period after a release
    void finishMove();
    /// a press (or a move with the button held) right after a release: the same drag goes on
    void resumeMove(QGraphicsSceneMouseEvent* event);
    QTimer* release_timer;  // the grace period after a release
    QPointF release_pos;
    QPoint release_screen;  // where it was released on the screen
    bool explicit_grab = false;
    QElapsedTimer other_press;      // the first press on it while of another aircraft (a double click selects it)
    QPoint other_press_screen;     // grabbed during the grace period (and the drag after it)    // where the point was released, in the scene
    double highlight_factor;    // map/size_highlight_factor (read once: not on every paint)
    int halfSize;
    QPointF pressPos;       // from the point to the mouse, in the scene
    QPointF pressScreen;
    bool hover_mark = false;
    std::optional<double> heading;
    bool double_pressed = false;    // the press is the second of a double click    // where it was pressed on the screen (move threshold)
    PointMoveState move_state;

    int current_color;

    int animation_couter;
};

#endif // GRAPHICSPOINT_H
