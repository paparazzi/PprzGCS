#ifndef ARROWITEM_H
#define ARROWITEM_H

#include <QObject>
#include "map_item.h"
#include "graphics_group.h"
#include <functional>
#include <QFont>
#include <QGraphicsItem>

/// The tag of an aircraft out of the view, at the edge of the map: its name and distance, pointing
/// towards it. Drawn in pixels (not scaled or turned with the map); a click selects the aircraft.
class OffscreenTag : public QGraphicsItem {
public:
    OffscreenTag();
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void set(QString text, QColor color, double angle_deg, bool selected);
    QSizeF pillSize() const;
    std::function<void()> clicked;
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
private:
    QPainterPath tagPath() const;
    QString text;
    QColor color;
    double angle = 0;       // towards the aircraft, in the view (deg, 0: right, 90: down)
    bool selected = true;
    QFont font;
};

class ArrowItem : public MapItem
{
    Q_OBJECT
//Q_PROPERTY(int size MEMBER m_size)
    Q_PROPERTY(int size MEMBER m_size DESIGNABLE true)
public:
    explicit ArrowItem(QString ac_id, double neutral_scale_zoom = 15, QObject *parent = nullptr);

    virtual void setHighlighted(bool h);
    virtual void updateZValue();
    virtual void setVisible(bool visible);
    virtual void setForbidHighlight(bool fh);
    virtual void setEditable(bool ed){(void)ed;}
    virtual void removeFromScene(MapWidget* map);
    virtual void addToMap(MapWidget* mw);

    void setAcPos(Point2DLatLon pos) {
        ac_pos = pos;
        emit itemChanged();
    }

signals:
    void centerAC();

protected:
    virtual void updateGraphics(MapWidget* map, uint32_t update_event);

private:

    int m_size;
    OffscreenTag* scene_item;
    bool shown = true;
    Point2DLatLon ac_pos;

};

#endif // ARROWITEM_H
