#include "mapwidget.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include "map_item.h"
#include <iostream>
#include <QPushButton>
#include <QLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QScrollArea>
#include <QDebug>
#include <QSpacerItem>
#include "maplayercontrol.h"
#include <QApplication>
#include "pprz_dispatcher.h"
#include "maputils.h"
#include "aircraft_item.h"
#include "dispatcher_ui.h"
#include "AircraftManager.h"
#include "widget_utils.h"
#include "layer_combo.h"
#include "lock_button.h"
#include "gcs_utils.h"

MapWidget::MapWidget(QWidget *parent) : Map2D(parent),
    pan_state(PAN_IDLE), pan_mouse_mask(Qt::MiddleButton | Qt::LeftButton)
{
    horizontalLayout = new QHBoxLayout(this);   // main layout

    buttonsLeftLayout = new QVBoxLayout();
    columnLeft = new QVBoxLayout();
    spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    columnRight = new QVBoxLayout();
    buttonsRightLayout = new QVBoxLayout();

    horizontalLayout->addItem(buttonsLeftLayout);
    horizontalLayout->addItem(columnLeft);
    horizontalLayout->addItem(spacer);
    horizontalLayout->addItem(columnRight);
    horizontalLayout->addItem(buttonsRightLayout);
    horizontalLayout->setStretch(2, 1);

    auto bspacer_left = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    buttonsLeftLayout->addItem(bspacer_left);
    buttonsLeftLayout->setStretch(0, 1);

    auto bspacer_right = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    buttonsRightLayout->addItem(bspacer_right);
    buttonsRightLayout->setStretch(0, 1);

    auto vspacer_left = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    columnLeft->addItem(vspacer_left);
    columnLeft->setStretch(0, 1);

    auto vspacer_right = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    columnRight->addItem(vspacer_right);
    columnRight->setStretch(0, 1);

    setDragMode(QGraphicsView::NoDrag);
    setMouseLoadTileMask(Qt::MiddleButton | Qt::LeftButton);

    setMouseTracking(true);

    setZoom(17);
    centerLatLon(Point2DLatLon(43.462344,1.273044));
    setTilesPath(qApp->property("MAP_PATH").toString());

    addLayersWidget();

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &MapWidget::handleNewAC);

}

void MapWidget::addLayersWidget() {
    auto layer_combo = new LayerCombo(this);
    //layer_combo->setStyleSheet("QWidget{background-color: #31363b;} QLabel{color:white;}");

    int i = tileProvidersNames().length();
    for(auto tp: tileProvidersNames() ) {
        bool shown = false;
        if(tp == qApp->property("DEFAULT_TILE_PROVIDER").toString()) {
            toggleTileProvider(tp, true, i, 1);
            shown = true;
        }
        auto lc = makeLayerControl(tp, shown, i);
        layer_combo->addLayerControl(lc);
        i--;
    }


    auto button = new LockButton(QIcon(qApp->property("APP_DATA_PATH").toString() + "/pictures/" + "map_layers_normal.svg"), this);

    addWidget(layer_combo, button, WIDGETS_LEFT);
}

void MapWidget::addWidget(QWidget* widget, LockButton* button, WidgetContainer side) {
    QList<LockButton*>* buttons;
    if(side == WIDGETS_LEFT) {
        columnLeft->insertWidget(columnLeft->count() - 1, widget);
        buttonsLeftLayout->insertWidget(buttonsLeftLayout->count()-1, button);
        button->setActiveSide(false);
        buttons = &buttonsLeft;
    } else  {
        columnRight->insertWidget(columnRight->count() - 1, widget);
        buttonsRightLayout->insertWidget(buttonsRightLayout->count()-1, button);
        button->setActiveSide(true);
        buttons = &buttonsRight;
        columnRight->setAlignment(widget, Qt::AlignRight);
    }

    widget->hide();
    button->setWidget(widget);
    buttons->append(button);

    connect(
        button, &LockButton::clicked,
        [=](bool active) {
            for(auto lb: *buttons) {
                if(lb != button) {  //other buttons
                    if(!lb->isLocked()) {
                        lb->widget()->hide();
                        lb->setActive(false);
                    }
                } else {    // this button
                    lb->widget()->setVisible(active);
                }
            }
        }
    );
}

void MapWidget::configure(QDomElement ele) {
    for(int i=0; i<ele.childNodes().length(); i++) {
        QDomNode node = ele.childNodes().item(i);
        if(!node.isElement()) {
            continue;
        }
        QDomElement child_ele = node.toElement();

        for(int i=0; i<child_ele.childNodes().length(); i++) {
            QDomNode w_node = child_ele.childNodes().item(i);
            if(!w_node.isElement()) {
                continue;
            }
            QDomElement w_ele = w_node.toElement();
            assert(w_ele.tagName() == "widget");
            auto name = w_ele.attribute("name");

            QWidget* widget = makeWidget(name, this);

            WidgetContainer side;
            if(child_ele.tagName() == "columnLeft") {
                side = WIDGETS_LEFT;
            } else if (child_ele.tagName() == "columnRight") {
                side = WIDGETS_RIGHT;
            } else {
                throw runtime_error("Unknow tag" + child_ele.tagName().toStdString());
            }

            auto icon_src = w_ele.attribute("icon");
            if(icon_src == "") {
                std::string s = "Missing attribute 'icon' for node " + child_ele.tagName().toStdString() + "!";
                throw runtime_error(s);
            }

            auto icon = QIcon(user_or_app_path("/pictures/" + icon_src));
            auto button = new LockButton(icon, this);

            addWidget(widget, button, side);
            installEventFilter(widget);

        }
    }
}

MapLayerControl* MapWidget::makeLayerControl(QString name, bool initialState, int z) {
    QString path = user_or_app_path("/pictures/map_thumbnails/" + name + ".png");

    QPixmap thumbnail = QPixmap(path);
    if(thumbnail.isNull()) {
        path = qApp->property("APP_DATA_PATH").toString() + "/pictures/map_thumbnails/default.png";
        thumbnail = QPixmap(path);
    }

    MapLayerControl* layer_control = new MapLayerControl(name, thumbnail, initialState, z);

    connect(
        layer_control, &MapLayerControl::showLayer, this,
        [=](bool state) {
            toggleTileProvider(name, state, layer_control->zValue(), layer_control->opacity());
            updateTiles();
        }
    );

    connect(
        layer_control, &MapLayerControl::layerOpacityChanged, this,
        [=](qreal opacity) {
            setLayerOpacity(name, opacity);
        }
    );

    connect(
        layer_control, &MapLayerControl::zValueChanged, this,
        [=](int z) {
            setLayerZ(name, z);
        }
    );

    return layer_control;
}


void MapWidget::setCursor(const QCursor &cur) {
    QGraphicsView::setCursor(cur);
//    leftScrollArea->setCursor(Qt::ArrowCursor);
//    layers_button->setCursor(Qt::ArrowCursor);
}

void MapWidget::addItem(MapItem* map_item) {
    map_item->updateGraphics();
    _items.append(map_item);
    emit(itemAdded(map_item));
}

// TODO Use shared_ptr ?
void MapWidget::removeItem(MapItem* item) {
    item->removeFromScene();
    _items.removeAll(item);
    emit(itemRemoved(item));
    //the one that receive this signal shall delete the item
}

void MapWidget::itemsForbidHighlight(bool fh) {
    for(auto item: _items) {
        item->setForbidHighlight(fh);
    }
}

void MapWidget::itemsEditable(bool ed) {
    for(auto item: _items) {
        item->setEditable(ed);
    }
}

void MapWidget::updateHighlights(QString ac_id) {
    for(auto item: _items) {
        if(item->acId() == ac_id) {
            item->setHighlighted(true);
            item->setZValue(qApp->property("ITEM_Z_VALUE_HIGHLIGHTED").toInt());
        } else {
            item->setHighlighted(false);
            item->setZValue(qApp->property("ITEM_Z_VALUE_UNHIGHLIGHTED").toInt());
        }
    }
}

void MapWidget::mousePressEvent(QMouseEvent *event) {
    Map2D::mousePressEvent(event);
    if(event->buttons() & pan_mouse_mask && !event->isAccepted()) {
        pan_state = PAN_PRESSED;
        lastPos = event->pos();
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent *event) {
    Map2D::mouseMoveEvent(event);
    if(event->buttons() & pan_mouse_mask) {
        if(pan_state == PAN_PRESSED) {
            QPoint dp = event->pos()-lastPos;
            double d = sqrt(dp.x()*dp.x() + dp.y()*dp.y());
            if(d > qApp->property("MAP_MOVE_HYSTERESIS").toInt()) {
                pan_state = PAN_MOVE;
            }
        } else if(pan_state == PAN_MOVE) {



            QPoint dp = event->pos()-lastPos;
            translate(dp.x()/scaleFactor(), dp.y()/scaleFactor());
            lastPos = event->pos();
        }
    }
    emit(mouseMoved(mapToScene(event->pos())));
}

void MapWidget::mouseReleaseEvent(QMouseEvent *event) {
    Map2D::mouseReleaseEvent(event);
    pan_state = PAN_IDLE;
}

void MapWidget::wheelEvent(QWheelEvent* event) {
    Map2D::wheelEvent(event);
    for(auto item: _items) {
        item->updateGraphics();
    }
}


void MapWidget::handleNewAC(QString ac_id) {
    (void)ac_id;
}
