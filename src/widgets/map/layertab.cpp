#include "layertab.h"
#include <QDebug>
#include <QMouseEvent>
#include <QApplication>

LayerTab::LayerTab(QWidget *parent) : QWidget(parent),
    moved_layer_control(nullptr)
{
    vbox_layout = new QVBoxLayout(this);
    moved_thumbnail = new QLabel();
    moved_thumbnail->setWindowFlags(Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint| Qt::Window);
    moved_thumbnail->setAttribute(Qt::WA_NoSystemBackground, true);
    moved_thumbnail->setAttribute(Qt::WA_TranslucentBackground, true);

}


void LayerTab::addLayerControl(QString name, MapLayerControl* layer_control) {
    map_layer_controls[name] = layer_control;
    vbox_layout->addWidget(layer_control);
}

MapLayerControl* LayerTab::layerControl(QString name) {
    if(map_layer_controls.find(name) != map_layer_controls.end()) {
        return map_layer_controls[name];
    } else {
        throw std::runtime_error("No such MapLayerControl as \"" + name.toStdString() + "\"");
    }
}


void LayerTab::mousePressEvent(QMouseEvent* e) {
    for(auto mlc: map_layer_controls) {
        if(mlc.second->underMouse()) {
            moved_layer_control = mlc.second;
            press_pos = e->pos();
            //qDebug() << mlc.first << " pressed at " << e->pos();

            moved_thumbnail->setPixmap(mlc.second->pixmap()->scaled(80,80));
            moved_thumbnail->resize(moved_thumbnail->pixmap()->size());
            moved_thumbnail->raise();

            QPoint globalPos = mapToGlobal(e->pos());
            QSize pixSize = moved_thumbnail->pixmap()->size();
            QPoint pos(globalPos.x()-pixSize.width()/2, globalPos.y()-pixSize.height()/2);
            moved_thumbnail->move(pos);
            moved_thumbnail->show();
        }
    }
}

void LayerTab::mouseMoveEvent(QMouseEvent* e) {
    (void) e;
    if(moved_layer_control) {
        QPoint globalPos = mapToGlobal(e->pos());
        QSize pixSize = moved_thumbnail->pixmap()->size();
        QPoint pos(globalPos.x()-pixSize.width()/2, globalPos.y()-pixSize.height()/2);
        moved_thumbnail->move(pos);
        //qDebug() << "moved at " << e->pos();
    }

}

void LayerTab::mouseReleaseEvent(QMouseEvent* e) {
    (void) e;
    if(moved_layer_control) {
        qDebug() << " released at " << e->pos().y();
        moved_thumbnail->hide();

        for(auto mlc: map_layer_controls) {
            qDebug() << mlc.first << ": " << mlc.second->pos().y();
        }


    }
    moved_layer_control = nullptr;
}
