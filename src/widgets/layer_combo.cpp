#include "layer_combo.h"

LayerCombo::LayerCombo(QWidget *parent) : QWidget(parent), moved_layer_control(nullptr)
{
    scroll = new QScrollArea();
    scroll_content = new QWidget();
    content_layout = new QVBoxLayout(scroll_content);
    main_layout  = new QVBoxLayout(this);
    main_layout->addWidget(scroll);
    scroll->setWidget(scroll_content);
    scroll->setWidgetResizable(true);

    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    moved_thumbnail = new QLabel();
    moved_thumbnail->setWindowFlag(Qt::SubWindow);
    moved_thumbnail->setWindowFlag(Qt::FramelessWindowHint);
    moved_thumbnail->setWindowFlag(Qt::WindowStaysOnTopHint);

}

void LayerCombo::addLayerControl(MapLayerControl* mlc) {
    mlc->setParent(this);
    content_layout->addWidget(mlc);
    map_layer_controls.append(mlc);
}

void LayerCombo::mousePressEvent(QMouseEvent* e) {
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

void LayerCombo::mouseMoveEvent(QMouseEvent* e) {
    (void) e;
    if(moved_layer_control) {
        QPoint globalPos = mapToGlobal(e->pos());
        QSize pixSize = moved_thumbnail->pixmap()->size();
        QPoint pos(globalPos.x()-pixSize.width()/2, globalPos.y()-pixSize.height()/2);
        moved_thumbnail->move(pos);
        qDebug() << "moved at " << e->pos();
        int scroll_y = e->pos().y();
        //int scroll_y = mapTo(scroll, e->pos()).y();
        qDebug() << "bbb";
        int scroll_value = scroll->verticalScrollBar()->value();
        qDebug() << "ccc";
        if(scroll_y < 30) {
            scroll_value = qMax(scroll_value - scroll_y, scroll->verticalScrollBar()->minimum());
            qDebug() << "scroll up";
            scroll->verticalScrollBar()->setValue(scroll_value);
        } else if(scroll->height() - scroll_y < 30) {
            qDebug() << "scroll down";
            scroll_value = qMin(scroll_value + scroll->height() - scroll_y, scroll->verticalScrollBar()->maximum());
            scroll->verticalScrollBar()->setValue(scroll_value);
        }
    }
}

void LayerCombo::mouseReleaseEvent(QMouseEvent* e) {
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

        content_layout->removeWidget(moved_layer_control);
        for(auto mlc: map_layer_controls) {
            content_layout->removeWidget(mlc);
        }

        for(auto mlc: map_layer_controls) {
            content_layout->addWidget(mlc);
        }

    }
    moved_layer_control = nullptr;
}
