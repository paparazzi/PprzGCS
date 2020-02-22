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
    void setForbidHighlight(bool fh) {forbid_highlight = fh;}
    void setEditable(bool ed) {editable = ed;}
    void setScaleFactor(double s) {scale_factor = s;}

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void changeFocus() = 0;
    bool forbid_highlight;
    bool editable;
    double scale_factor;

signals:
    void objectClicked(QPointF scene_pos);
    void objectGainedHighlight();

private:
    bool highlighted;

private:


};

#endif // GRAPHICSOBJECT_H
