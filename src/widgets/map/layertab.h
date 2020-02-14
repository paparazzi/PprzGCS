#ifndef LAYERTAB_H
#define LAYERTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include "maplayercontrol.h"

class LayerTab : public QWidget
{
    Q_OBJECT
public:
    explicit LayerTab(QWidget *parent = nullptr);

    void addLayerControl(QString name, MapLayerControl* layerControl);
    MapLayerControl* layerControl(QString name);
    std::map<QString, MapLayerControl*>& layerControls() {return map_layer_controls;}

protected:
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

signals:

public slots:

private:
    QVBoxLayout* vbox_layout;
    std::map<QString, MapLayerControl*> map_layer_controls;

    MapLayerControl* moved_layer_control;
    QPoint press_pos;
    QLabel* moved_thumbnail;

};

#endif // LAYERTAB_H
