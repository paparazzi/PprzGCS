#ifndef PAPGET_H
#define PAPGET_H

#include <QObject>
#include <QGraphicsItem>
#include "pprz_dispatcher.h"

class MapWidget;

class Papget : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:

    struct DataDef {
        QString ac_id;
        QString msg_name;
        QString field;
    };

    enum Style {
        TEXT,
    };

    explicit Papget(struct DataDef datadef, QPoint pos_view, QObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    //QPainterPath shape() const override;
    void updateGraphics(MapWidget* map);
    void setPosition(QPoint viewPos) {pos_view = viewPos;}

signals:
    void moved(QPointF);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private:

    enum Type {
        NONE,
        U32,
        I32,
        FLOAT,
        STR,
        CHAR,
    };

    enum MoveState {
        IDLE,
        PRESSED,
        MOVED,
    };

    struct DataDef datadef;
    long bindRet;

    void callback(std::string, pprzlink::Message);

    template<typename T, typename U>
    bool try_this(pprzlink::Message, U &dst);

    void paint_text(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    Type type;
    Style style;

    uint32_t val_u32;
    int32_t val_32;
    std::string val_str;
    double val_float;
    char val_char;

    QRectF bounding_rect;
    double scale_factor;
    QPoint pos_view;
    QPointF pressPos;
    MoveState move_state;

};

#endif // PAPGET_H
