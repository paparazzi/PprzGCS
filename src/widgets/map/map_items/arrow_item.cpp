#include "arrow_item.h"
#include "mapwidget.h"
#include "gcs_utils.h"
#include "dispatcher_ui.h"


#include "AircraftManager.h"
#include "coordinatestransform.h"
#include <QGraphicsSceneMouseEvent>

namespace {
constexpr double TIP = 0;       // (nothing out of the pill)
constexpr double ARROW = 16;    // room for the arrow in the tag
constexpr double MARGIN = 8;    // from the edge of the visible map

double distanceM(Point2DLatLon a, Point2DLatLon b) {
    double la1 = qDegreesToRadians(a.lat()), la2 = qDegreesToRadians(b.lat());
    double dla = la2 - la1, dlo = qDegreesToRadians(b.lon() - a.lon());
    double h = sin(dla / 2) * sin(dla / 2) + cos(la1) * cos(la2) * sin(dlo / 2) * sin(dlo / 2);
    return 2 * 6371000.0 * atan2(sqrt(h), sqrt(1 - h));
}
}

OffscreenTag::OffscreenTag() {
    setFlag(QGraphicsItem::ItemIgnoresTransformations);     // in pixels, upright
    setAcceptedMouseButtons(Qt::LeftButton);
    setCursor(Qt::PointingHandCursor);
    font.setPixelSize(12);
    font.setBold(true);
}

QSizeF OffscreenTag::pillSize() const {
    QFontMetricsF fm(font);
    return QSizeF(fm.horizontalAdvance(text) + 20 + ARROW, 24);    // (the arrow before the text)
}

QPainterPath OffscreenTag::tagPath() const {
    QSizeF sz = pillSize();
    QPainterPath path;
    path.addRoundedRect(QRectF(-sz.width() / 2, -sz.height() / 2, sz.width(), sz.height()), sz.height() / 2, sz.height() / 2);
    return path;
}

QRectF OffscreenTag::boundingRect() const {
    QSizeF sz = pillSize();
    double m = TIP + 4;
    return QRectF(-sz.width() / 2 - m, -sz.height() / 2 - m, sz.width() + 2 * m, sz.height() + 2 * m);
}

QPainterPath OffscreenTag::shape() const {
    return tagPath();
}

void OffscreenTag::set(QString t, QColor c, double a, bool s) {
    if(t != text) {
        prepareGeometryChange();
    }
    text = t;
    color = c;
    angle = a;
    selected = s;
    update();
}

void OffscreenTag::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    (void)option;
    (void)widget;
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(selected ? 1.0 : 0.8);
    auto path = tagPath();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 80));
    painter->drawPath(path.translated(0, 1.5));
    painter->setPen(QPen(Qt::white, 2));
    painter->setBrush(color);
    painter->drawPath(path);
    // white text, or dark on a light color
    painter->setPen(color.lightnessF() > 0.7 ? QColor(0x1f2328) : QColor(Qt::white));
    painter->setFont(font);
    QSizeF sz = pillSize();
    QColor ink = painter->pen().color();
    painter->drawText(QRectF(-sz.width() / 2 + ARROW, -sz.height() / 2, sz.width() - ARROW, sz.height()), Qt::AlignCenter, text);
    // the arrow before the name, towards the aircraft
    painter->save();
    painter->translate(-sz.width() / 2 + 6 + ARROW / 2, 0);
    painter->rotate(angle + 90);
    painter->setPen(Qt::NoPen);
    painter->setBrush(ink);
    painter->drawPolygon(QPolygonF({QPointF(0, -7), QPointF(5.5, 6), QPointF(0, 2.5), QPointF(-5.5, 6)}));
    painter->restore();
}

void OffscreenTag::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    event->accept();
    if(clicked) {
        clicked();
    }
}


ArrowItem::ArrowItem(QString ac_id, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, neutral_scale_zoom, parent), m_size(30)
{
    auto settings = getAppSettings();

    z_value_highlighted = settings.value("map/z_values/aircraft").toDouble();
    z_value_unhighlighted = settings.value("map/z_values/aircraft").toDouble();
    z_value = z_value_unhighlighted;

    scene_item = new OffscreenTag();
    scene_item->clicked = [=,this]() {
        emit centerAC();
        emit DispatcherUi::get()->ac_selected(ac_id);
    };
    scene_item->hide();
    setZoomFactor(1.1);
}


void ArrowItem::setHighlighted(bool h) {
    MapItem::setHighlighted(h);
    updateZValue();
}

void ArrowItem::updateZValue() {
    scene_item->setZValue(z_value + (highlighted ? 1 : 0));
}

void ArrowItem::setVisible(bool visible) {
    shown = visible;    // (shown only while the aircraft is out of the view)
    if(!visible) {
        scene_item->hide();
    }
}

void ArrowItem::setForbidHighlight(bool fh) {
    (void)fh;
}

void ArrowItem::removeFromScene(MapWidget* map) {
    assert(scene_item != nullptr);
    map->scene()->removeItem(scene_item);
    delete scene_item;
    scene_item = nullptr;
}
void ArrowItem::addToMap(MapWidget* map) {
    map->scene()->addItem(scene_item);
}

void ArrowItem::updateGraphics(MapWidget* map, uint32_t update_event) {
    if(!(update_event & UpdateEvent::ANY)) {
        return;
    }
    // the visible map: not under the rail on the left
    QRectF view = QRectF(map->rect());
    if(auto rail = map->findChild<QWidget*>("map_rail")) {
        if(rail->isVisible()) {
            view.setLeft(rail->geometry().right() + 1);
        }
    }
    QPointF ac_scene_pos = scenePoint(ac_pos, zoomLevel(map->zoom()), map->tileSize());
    QPointF p = map->mapFromScene(ac_scene_pos);
    if(!shown || view.contains(p) || (ac_pos.lat() == 0 && ac_pos.lon() == 0)) {
        scene_item->hide();
        return;
    }
    QString name = AircraftManager::get()->aircraftExists(ac_id) ? AircraftManager::get()->getAircraft(ac_id)->name() : ac_id;
    // the distance from the middle of the view
    QPointF c = view.center();
    auto center = CoordinatesTransform::get()->wgs84_from_scene(map->mapToScene(c.toPoint()), zoomLevel(map->zoom()), map->tileSize());
    double d = distanceM(center, ac_pos);
    QString dist = d < 1000 ? QString("%1 m").arg(d, 0, 'f', 0) : QString("%1 km").arg(d / 1000, 0, 'f', d < 10000 ? 1 : 0);
    double angle = qRadiansToDegrees(atan2(p.y() - c.y(), p.x() - c.x()));
    scene_item->set(name + QString::fromUtf8(" · ") + dist, palette.getColor(), angle, highlighted);

    // on the line from the middle to the aircraft, as far as the tag stays in the view
    QSizeF sz = scene_item->pillSize();
    double hw = view.width() / 2 - sz.width() / 2 - TIP - MARGIN;
    double hh = view.height() / 2 - sz.height() / 2 - TIP - MARGIN;
    double dx = p.x() - c.x(), dy = p.y() - c.y();
    double t = std::min(fabs(dx) > 1e-6 ? hw / fabs(dx) : 1e9, fabs(dy) > 1e-6 ? hh / fabs(dy) : 1e9);
    QPointF pos = c + QPointF(dx, dy) * std::max(0.0, t);
    scene_item->setPos(map->mapToScene(pos.toPoint()));
    scene_item->show();
}
