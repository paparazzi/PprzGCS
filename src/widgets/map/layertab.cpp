#include "layertab.h"
#include <QDebug>
#include <QMouseEvent>
#include <QApplication>
#include <QScrollBar>

LayerTab::LayerTab(QScrollArea* scroll_area, QWidget *parent) : QWidget(parent),
    moved_layer_control(nullptr), scroll_container(scroll_area)
{
    vbox_layout = new QVBoxLayout(this);
    moved_thumbnail = new QLabel();
    moved_thumbnail->setWindowFlags(Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint| Qt::Window);
    moved_thumbnail->setAttribute(Qt::WA_NoSystemBackground, true);
    moved_thumbnail->setAttribute(Qt::WA_TranslucentBackground, true);

}


void LayerTab::addLayerControl(MapLayerControl* layer_control) {
    map_layer_controls.append(layer_control);
    vbox_layout->addWidget(layer_control);
}

MapLayerControl* LayerTab::layerControl(QString name) {
    for(auto mlc: map_layer_controls) {
        if(name == mlc->name()) {
            return mlc;
        }
    }
    throw std::runtime_error("No such MapLayerControl as \"" + name.toStdString() + "\"");
}


void LayerTab::mousePressEvent(QMouseEvent* e) {
    for(auto mlc: map_layer_controls) {
        if(mlc->underMouse()) {
            moved_layer_control = mlc;
            press_pos = e->pos();

            QSize pSize = mlc->pixmap()->size();
            QSize pixSize = QSize(pSize.width()/2, pSize.height()/2);
            moved_thumbnail->setPixmap(mlc->pixmap()->scaled(pixSize));
            //moved_thumbnail->resize(moved_thumbnail->pixmap()->size());
            moved_thumbnail->raise();

            QPoint globalPos = mapToGlobal(e->pos());
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
        int scroll_y = mapTo(scroll_container, e->pos()).y();
        int scroll_value = scroll_container->verticalScrollBar()->value();
        if(scroll_y < 30) {
            scroll_value = qMax(scroll_value - scroll_y, scroll_container->verticalScrollBar()->minimum());
            qDebug() << "scroll up";
            scroll_container->verticalScrollBar()->setValue(scroll_value);
        } else if(scroll_container->height() - scroll_y < 30) {
            qDebug() << "scroll down";
            scroll_value = qMin(scroll_value + scroll_container->height() - scroll_y, scroll_container->verticalScrollBar()->maximum());
            scroll_container->verticalScrollBar()->setValue(scroll_value);
        }
    }

}

void LayerTab::mouseReleaseEvent(QMouseEvent* e) {
    if(moved_layer_control) {
        moved_thumbnail->hide();
        int release_y = e->pos().y();
        int old_z = moved_layer_control->zValue();

        auto top_mlc = std::max_element(map_layer_controls.begin(), map_layer_controls.end(),
                                     [](MapLayerControl* lmlc, MapLayerControl* rmlc) {
                                           return lmlc->zValue() < rmlc->zValue();
                           }).i->t();

        map_layer_controls.removeAll(moved_layer_control);

        int new_z = top_mlc->zValue();


        for(auto mlc: map_layer_controls) {
            int pos_y = mlc->pos().y() + mlc->size().height()/2;
            if(release_y > pos_y) {
                new_z--;
            }
            if(mlc->zValue() > old_z && mlc->zValue() <= new_z) {
                mlc->setZValue(mlc->zValue() - 1);  //go down
            }
            else if(mlc->zValue() < old_z && mlc->zValue() >= new_z) {
                mlc->setZValue(mlc->zValue() + 1);  //go up
            }

        }

        moved_layer_control->setZValue(new_z);
        map_layer_controls.append(moved_layer_control);
        std::sort(map_layer_controls.begin(), map_layer_controls.end(),
                  [](MapLayerControl* lmlc, MapLayerControl* rmlc) {
                        return lmlc->zValue() > rmlc->zValue();
        });

        vbox_layout->removeWidget(moved_layer_control);
        for(auto mlc: map_layer_controls) {
            vbox_layout->removeWidget(mlc);
        }

        for(auto mlc: map_layer_controls) {
            vbox_layout->addWidget(mlc);
        }

    }
    moved_layer_control = nullptr;
}
