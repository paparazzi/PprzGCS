#ifndef GRAPHICSPOINT_H
#define GRAPHICSPOINT_H

#include <QObject>
#include <QGraphicsItem>
#include "graphics_object.h"
#include <QBrush>
#include "pprzpalette.h"

enum PointMoveState {
    PMS_IDLE,
    PMS_PRESSED,
    PMS_MOVED,
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

signals:
    /// pressed (also on its name); ctrl: the Ctrl key held
    void pointPressed(QPointF scenePos, bool ctrl);
    /// the mouse over its name (hoverChanged: over the point)
    void nameHoverChanged(bool hovered);
    void pointMoved(QPointF scenePos);
    void pointMoveFinished(QPointF scenePos);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void changeFocus() override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    /// the mouse on its name (installSceneEventFilter): handled as on the point
    bool sceneEventFilter(QGraphicsItem* watched, QEvent* event) override;



public slots:

private:
    int halfSize;
    QPointF pressPos;       // from the point to the mouse, in the scene
    QPointF pressScreen;
    bool hover_mark = false;
    bool double_pressed = false;    // the press is the second of a double click    // where it was pressed on the screen (move threshold)
    PointMoveState move_state;

    int current_color;

    int animation_couter;
};

#endif // GRAPHICSPOINT_H
