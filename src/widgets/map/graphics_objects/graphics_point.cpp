#include "graphics_point.h"
#include "math.h"
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QGraphicsScene>
#include <QCursor>
#include <QElapsedTimer>
#include "gcs_utils.h"

#define COLOR_IDLE 0
#define COLOR_PRESSED 1
#define COLOR_MOVED 2
#define COLOR_UNFOCUSED 3

GraphicsPoint::GraphicsPoint(int size, PprzPalette palette, QObject *parent) :
    GraphicsObject(palette, parent),
    QGraphicsItem (),
    halfSize(size), move_state(PMS_IDLE), animation_couter(0)
{
    current_color = COLOR_IDLE;
    highlight_factor = getAppSettings().value("map/size_highlight_factor").toDouble();
    if(highlight_factor <= 0) {
        highlight_factor = 1;
    }

    // Released while dragged: the card opens at once, but for a short time a press (or a move with
    // the button held) near it goes on with the same drag (its card closed). The mouse can lose
    // its button for a moment in the middle of a drag (seen with XWayland: a release and a new
    // press while the button is held).
    release_timer = new QTimer(this);
    release_timer->setSingleShot(true);
    release_timer->setInterval(400);
    connect(release_timer, &QTimer::timeout, this, [=,this]() {
        if(move_state == PMS_RELEASED) {
            finishMove();
        }
    });

    animation_timer->setInterval(500);
    connect(animation_timer, &QTimer::timeout, this, [=,this]()
    {
        animation_couter += 1;
        prepareGeometryChange();
    });
}

QRectF GraphicsPoint::boundingRect() const {
    // with the white border and the shadow
    return QRectF(-halfSize-4, -halfSize-4, 2*halfSize+8, 2*halfSize+9);
}


void GraphicsPoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    (void)option;
    (void)widget;

    if(hover_mark && style == DEFAULT) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(QColor(0x4da3ff), 2.5));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QPointF(0, 0), halfSize + 2, halfSize + 2);
        painter->restore();
    }
    if(animation == WP_MOVING) {
        painter->rotate(animation_couter * 45 + 1);
    }

    switch (style) {
    case DEFAULT: {
        QPainterPath path;
        // a square diamond
        double fx = 0.9;
        double fy = 0.9;
        if(!isHighlighted()) {
            current_color = COLOR_UNFOCUSED;
            fx /= highlight_factor;
            fy /= highlight_factor;
        }

        QPolygonF poly;
        poly.append(QPointF(0, halfSize*fy));
        poly.append(QPointF(halfSize*fx, 0));
        poly.append(QPointF(0, -halfSize*fy));
        poly.append(QPointF(-halfSize*fx, 0));
        poly.append(QPointF(0, halfSize*fy));


        path.addPolygon(poly);

        // a shadow, then the waypoint with a white border: visible on any background
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 90));
        painter->drawPath(path.translated(0, 1.5));
        painter->setPen(QPen(QColor(255, 255, 255, isHighlighted() ? 255 : 170), 2));
        painter->setBrush(QBrush(palette.getVariant(current_color)));
        painter->drawPath(path);
        }
        break;
    case CARROT: {
        QPainterPath path;
        QPolygonF poly;
        poly.append(QPointF(halfSize*cos(M_PI/6), -halfSize*sin(M_PI/6)));
        poly.append(QPointF(halfSize*cos(5*M_PI/6), -halfSize*sin(5*M_PI/6)));
        poly.append(QPointF(halfSize*cos(3*M_PI/2), -halfSize*sin(3*M_PI/2)));

        path.addPolygon(poly);
        // the navigation target: orange, with a shadow and a white border like the waypoints
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 90));
        painter->drawPath(path.translated(0, 1.5));
        painter->setPen(QPen(Qt::white, 1.5));
        painter->setBrush(QBrush(QColor(255, 170, 0)));
        painter->drawPath(path);
        }
        break;
    case CURRENT_NAV: {
        //draw nothing
        }
        break;
    case GCS: {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(palette.getVariant(current_color), 3));
        painter->drawEllipse(QPoint(0,0), 2*halfSize/3, 2*halfSize/3);
        painter->drawLine(0, -halfSize, 0, halfSize);
        painter->drawLine(-halfSize, 0, halfSize, 0);
        }
        break;
    case DCSHOT: {
        painter->setBrush(QBrush(palette.getVariant(current_color)));
        painter->drawEllipse(QPoint(0, 0), halfSize/2, halfSize/2);
        }
        break;
    case CRASH: {
        // Draw a red cross with thickness 3
        painter->setPen(QPen(QColor(255, 0, 0), 3));
        painter->drawLine(QPoint(-halfSize/2, -halfSize/2), QPoint(halfSize/2, halfSize/2));
        painter->drawLine(QPoint(-halfSize/2, halfSize/2), QPoint(halfSize/2, -halfSize/2));
        }
        break;
    }

}

QPainterPath GraphicsPoint::shape() const {
    if(style == CURRENT_NAV || style == CARROT) {
        QPainterPath path;
        return path;
    } else {
        return QGraphicsItem::shape();
    }
}

void GraphicsPoint::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    if(move_state == PMS_CARRIED) {
        event->accept();
        if(event->button() == Qt::LeftButton) {
            // placed here: as a drag, released with this click (the card then)
            setPos(event->scenePos());
            emit pointMoved(event->scenePos());
            pressPos = QPointF(0, 0);
            pressScreen = event->screenPos();
            move_state = PMS_MOVED;
            changeFocus();
        } else {
            cancelCarry();
        }
        return;
    }
    if(move_state == PMS_RELEASED) {
        // near where it was let go: the same drag (a press elsewhere is not for it)
        if(event->button() == Qt::LeftButton && (event->screenPos() - release_screen).manhattanLength() < 60) {
            resumeMove(event);     // pressed again right after the release: the same drag
        } else {
            finishMove();
            event->ignore();
        }
        return;
    }
    if(event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    if(move_state == PMS_MOVED) {
        return;     // a second press during the drag (it goes on)
    }
    if(!isHighlighted() && !(event->modifiers() & Qt::ControlModifier)) {
        // a waypoint of another aircraft: a press goes on to the map (panning), a double click
        // selects that aircraft (as on the aircraft); not moved, no card. With Ctrl: as the
        // others (Ctrl+click: the menu of the waypoints at that place)
        bool second = other_press.isValid() &&
                      other_press.elapsed() < QApplication::doubleClickInterval() &&
                      (event->screenPos() - other_press_screen).manhattanLength() < 8;
        if(second) {
            other_press.invalidate();
            GraphicsObject::mousePressEvent(event);     // selects its aircraft
            event->accept();
        } else {
            other_press.start();
            other_press_screen = event->screenPos();
            event->ignore();
        }
        return;
    }
    GraphicsObject::mousePressEvent(event);
    // in the scene: also right when pressed on its name, away from the point
    pressPos = event->scenePos() - scenePos();
    pressScreen = event->screenPos();
    move_state = PMS_PRESSED;
    double_pressed = false;
    changeFocus();
    emit pointPressed(event->scenePos(), event->modifiers() & Qt::ControlModifier);
}

bool GraphicsPoint::sceneEventFilter(QGraphicsItem* watched, QEvent* event) {
    (void)watched;
    if(ignore_events) {
        return false;
    }
    auto me = static_cast<QGraphicsSceneMouseEvent*>(event);
    switch(event->type()) {
    case QEvent::GraphicsSceneMousePress:
        if(me->button() != Qt::LeftButton && move_state != PMS_RELEASED) {
            return false;
        }
        mousePressEvent(me);
        return me->isAccepted();
    case QEvent::GraphicsSceneMouseMove:
        mouseMoveEvent(me);
        return true;
    case QEvent::GraphicsSceneMouseRelease:
        mouseReleaseEvent(me);
        return true;
    case QEvent::GraphicsSceneMouseDoubleClick:
        mouseDoubleClickEvent(me);
        return true;
    default:
        return false;
    }
}

void GraphicsPoint::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    if(move_state == PMS_PRESSED) {
        QPointF dp = event->screenPos() - pressScreen;
        double d = sqrt(dp.x()*dp.x() + dp.y()*dp.y());
        if(d > getAppSettings().value("map/move_hyteresis").toInt() && editable) {
            move_state = PMS_MOVED;
            changeFocus();
        }
    } else if(move_state == PMS_MOVED) {
        setPos(event->scenePos() - pressPos);
        emit pointMoved(event->scenePos() - pressPos);
    } else if(move_state == PMS_CARRIED) {
        setPos(event->scenePos());
        emit pointMoved(event->scenePos());
    } else if(move_state == PMS_RELEASED && (event->buttons() & Qt::LeftButton) &&
              (event->screenPos() - release_screen).manhattanLength() < 60) {
        resumeMove(event);     // the button is held again (the release was not real)
    }
}

void GraphicsPoint::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    if(event->button() != Qt::LeftButton || (event->buttons() & Qt::LeftButton)) {
        return;     // another button, or the left one still held: not the end of the drag
    }
    if(move_state == PMS_PRESSED) {
        move_state = PMS_IDLE;
        changeFocus();
        if(double_pressed) {
            double_pressed = false;
            emit objectDoubleClicked(event->scenePos());
        } else {
            emit objectClicked(event->scenePos());
        }
    }
    else if(move_state == PMS_MOVED) {
        // finished after the grace period, unless the drag goes on (pressed again). Grabbed
        // until then: a new press comes here even if it is a bit away from the point
        move_state = PMS_RELEASED;
        release_pos = event->scenePos() - pressPos;
        release_screen = event->screenPos();
        if(!explicit_grab) {
            explicit_grab = true;
            grabMouse();    // (the grab of the press made explicit: kept after the release)
        }
        release_timer->start();
        emit pointMoveFinished(release_pos);
    }
}

void GraphicsPoint::resumeMove(QGraphicsSceneMouseEvent* event) {
    release_timer->stop();
    move_state = PMS_MOVED;
    // the point where it is now, the mouse where it is now: no jump
    pressPos = event->scenePos() - scenePos();
    event->accept();    // (still grabbed: the next release comes here too)
    emit pointMoveResumed();
}

/// The end of the grace period after a release
void GraphicsPoint::finishMove() {
    release_timer->stop();
    move_state = PMS_IDLE;
    double_pressed = false;
    if(explicit_grab) {
        explicit_grab = false;
        if(scene() != nullptr && scene()->mouseGrabberItem() == this) {
            ungrabMouse();
        }
    }
    changeFocus();
}

/// The mouse taken away (by Qt, or another item): never stuck pressed or dragging
bool GraphicsPoint::sceneEvent(QEvent* event) {
    if(event->type() == QEvent::UngrabMouse) {
        mouseUngrabbed();
    }
    return QGraphicsItem::sceneEvent(event);
}

void GraphicsPoint::startCarry() {
    if(move_state != PMS_IDLE || ignore_events || !editable) {
        return;
    }
    release_timer->stop();
    move_state = PMS_CARRIED;
    pressPos = QPointF(0, 0);   // under the mouse
    if(!explicit_grab) {
        explicit_grab = true;
        grabMouse();    // the mouse moves (no button) and the click come here
    }
    changeFocus();
}

void GraphicsPoint::cancelCarry() {
    if(move_state != PMS_CARRIED) {
        return;
    }
    move_state = PMS_IDLE;
    if(explicit_grab) {
        explicit_grab = false;
        if(scene() != nullptr && scene()->mouseGrabberItem() == this) {
            ungrabMouse();
        }
    }
    changeFocus();
    emit pointMoveCancelled();
}

void GraphicsPoint::mouseUngrabbed() {
    explicit_grab = false;
    if(move_state == PMS_CARRIED) {
        move_state = PMS_IDLE;      // the mouse taken away: put back
        changeFocus();
        emit pointMoveCancelled();
        return;
    }
    if(move_state == PMS_PRESSED) {
        move_state = PMS_IDLE;
        double_pressed = false;
        changeFocus();
    } else if(move_state == PMS_MOVED) {
        // lost in the middle of the drag: as released here (a press on it goes on with it)
        move_state = PMS_RELEASED;
        release_pos = scenePos();
        release_screen = QCursor::pos();
        release_timer->start();
        emit pointMoveFinished(release_pos);
    }
}

/// The second press of a double click: the card once released without moving (a quick second
/// press that drags is a drag, the card opens when it is released)
void GraphicsPoint::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    if(move_state != PMS_IDLE) {
        mousePressEvent(event);     // during a drag (or its grace period): not a double click
        return;
    }
    if(!isHighlighted()) {
        // a waypoint of another aircraft: selects that aircraft only
        other_press.invalidate();
        GraphicsObject::mousePressEvent(event);
        event->accept();
        return;
    }
    mousePressEvent(event);
    double_pressed = move_state == PMS_PRESSED;
}

void GraphicsPoint::changeFocus() {
    if(!isHighlighted()) {
        current_color = COLOR_UNFOCUSED;
    } else {
        switch (move_state) {
        case PMS_IDLE:
            current_color = COLOR_IDLE;
            break;
        case PMS_PRESSED:
            current_color = COLOR_PRESSED;
            break;
        case PMS_MOVED:
        case PMS_RELEASED:
        case PMS_CARRIED:
            current_color = COLOR_MOVED;
            break;
        }
    }
    update();
}
