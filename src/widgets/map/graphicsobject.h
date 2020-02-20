#ifndef GRAPHICSOBJECT_H
#define GRAPHICSOBJECT_H

#include <QObject>
#include <QGraphicsSceneMouseEvent>
#include <QColor>

class GraphicsObject : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsObject(QObject *parent = nullptr);
    bool isHighlighted() {return highlighted;}
    virtual void setHighlighted(bool h);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void changeFocus() = 0;

signals:
    void objectClicked(QPointF scene_pos);
    void objectGainedHighlight();

private:
    bool highlighted;

private:


};

#endif // GRAPHICSOBJECT_H
