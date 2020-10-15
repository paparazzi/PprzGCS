#ifndef LAYER_COMBO_H
#define LAYER_COMBO_H

#include <QWidget>
#include <QtWidgets>
#include "maplayercontrol.h"

class LayerCombo : public QWidget
{
    Q_OBJECT
public:
    explicit LayerCombo(QWidget *parent = nullptr);

    void addLayerControl(MapLayerControl* mlc);
    bool eventFilter(QObject *object, QEvent *event);

    void setPopo(QPoint p) {_pos = p;}

signals:

public slots:

protected:
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

private:
    QScrollArea* scroll;
    QWidget* scroll_content;
    QVBoxLayout* content_layout;
    QVBoxLayout* main_layout;

    QPoint _pos;

    QList<MapLayerControl*> map_layer_controls;

    MapLayerControl* moved_layer_control;
    QPoint press_pos;
    QLabel* moved_thumbnail;
};

#endif // LAYER_COMBO_H
