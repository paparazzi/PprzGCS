#ifndef SMOOTH_ITEM_H
#define SMOOTH_ITEM_H

#include <QPainter>
#include <QStyleOptionGraphicsItem>

/// A Qt graphics item drawn antialiased (the map is not: its tiles and large shapes are drawn
/// faster and without seams). E.g. SmoothItem<QGraphicsLineItem> for a thin line.
template<class T>
class SmoothItem : public T
{
public:
    using T::T;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        T::paint(painter, option, widget);
        painter->restore();
    }
};

#endif // SMOOTH_ITEM_H
