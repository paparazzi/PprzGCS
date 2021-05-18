#include "mapwidget.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include "map_item.h"
#include <iostream>
#include <QPushButton>
#include <QLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QDebug>
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
#include <QSettings>

MapWidget::MapWidget(QWidget *parent) : Map2D(parent),
    pan_state(PAN_IDLE), pan_mouse_mask(Qt::MiddleButton | Qt::LeftButton)
{
    auto settings = getAppSettings();

    horizontalLayout = new QHBoxLayout(this);   // main layout
    buttonsLeftLayout = new QVBoxLayout();
    columnLeft = new QVBoxLayout();
    columnRight = new QVBoxLayout();
    buttonsRightLayout = new QVBoxLayout();

    horizontalLayout->addItem(buttonsLeftLayout);
    horizontalLayout->addItem(columnLeft);
    horizontalLayout->addStretch(1);
    horizontalLayout->addItem(columnRight);
    horizontalLayout->addItem(buttonsRightLayout);

    buttonsLeftLayout->addStretch(1);
    buttonsRightLayout->addStretch(1);
    columnLeft->addStretch(1);
    columnRight->addStretch(1);

    setDragMode(QGraphicsView::NoDrag);
    setMouseLoadTileMask(Qt::MiddleButton | Qt::LeftButton);

    setMouseTracking(true);

    setZoom(2);
    centerLatLon(Point2DLatLon(43.462344,1.273044));
    setTilesPath(settings.value("map/tiles_path").toString());

    addLayersWidget();

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &MapWidget::handleNewAC);
    setAcceptDrops(true);
}

void MapWidget::addLayersWidget() {
    auto settings = getAppSettings();

    auto layer_combo = new LayerCombo(this);
    //layer_combo->setStyleSheet("QWidget{background-color: #31363b;} QLabel{color:white;}");

    int i = tileProvidersNames().length();
    for(auto &tp: tileProvidersNames() ) {
        bool shown = false;
        if(tp == settings.value("map/default_tiles").toString()) {
            toggleTileProvider(tp, true, i, 1);
            shown = true;
        }
        auto lc = makeLayerControl(tp, shown, i);
        layer_combo->addLayerControl(lc);
        i--;
    }

    auto button = new LockButton(QIcon(settings.value("APP_DATA_PATH").toString() + "/pictures/" + "map_layers_normal.svg"), this);

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

            auto icon = QIcon(user_or_app_path("pictures/" + icon_src));
            auto button = new LockButton(icon, this);

            addWidget(widget, button, side);
            installEventFilter(widget);

        }
    }
}

MapLayerControl* MapWidget::makeLayerControl(QString name, bool initialState, int z) {
    QString path = user_or_app_path("pictures/map_thumbnails/" + name + ".png");
    auto settings = getAppSettings();
    QPixmap thumbnail = QPixmap(path);
    if(thumbnail.isNull()) {
        path = user_or_app_path("pictures/map_thumbnails/default.png");
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
    map_item->addToMap(this);
    map_item->updateGraphics(this);
    _items.append(map_item);
    emit(itemAdded(map_item));

    connect(map_item, &MapItem::itemChanged, map_item, [=]() {
        map_item->updateGraphics(this);
    });

    connect(map_item, &MapItem::itemGainedHighlight, map_item, [=]() {
        QString ac_id = map_item->acId();
        emit(DispatcherUi::get()->ac_selected(ac_id));
    });
}

// TODO Use shared_ptr ?
void MapWidget::removeItem(MapItem* item) {
    item->removeFromScene(this);
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
        } else {
            item->setHighlighted(false);
        }
    }
}

void MapWidget::centerLatLon(Point2DLatLon latLon) {
    Map2D::centerLatLon(latLon);
    for(auto papget: papgets) {
        papget->updateGraphics(this);
    }
}

void MapWidget::setZoom(double z) {
    Map2D::setZoom(z);
    for(auto papget: papgets) {
        papget->updateGraphics(this);
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
    auto settings = getAppSettings();
    if(event->buttons() & pan_mouse_mask) {
        if(pan_state == PAN_PRESSED) {
            QPoint dp = event->pos()-lastPos;
            double d = sqrt(dp.x()*dp.x() + dp.y()*dp.y());
            if(d > settings.value("map/move_hyteresis").toInt()) {
                pan_state = PAN_MOVE;
            }
        } else if(pan_state == PAN_MOVE) {
            QPoint dp = event->pos()-lastPos;
            translate(dp.x()/scaleFactor(), dp.y()/scaleFactor());
            lastPos = event->pos();
        }

        for(auto papget: papgets) {
            papget->updateGraphics(this);
        }
    }
    emit mouseMoved(mapToScene(event->pos()));
}

void MapWidget::mouseReleaseEvent(QMouseEvent *event) {
    Map2D::mouseReleaseEvent(event);
    pan_state = PAN_IDLE;
}

void MapWidget::wheelEvent(QWheelEvent* event) {
    Map2D::wheelEvent(event);
    for(auto item: _items) {
        item->updateGraphics(this);
    }
    for(auto papget: papgets) {
        papget->updateGraphics(this);
    }
}

void MapWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat("text/plain")) {
        event->acceptProposedAction();
    }
}

void MapWidget::dragMoveEvent(QDragMoveEvent *event) {
    event->accept();
}

/**
 * @brief MapWidget::dropEvent handle drop for mime type text/plain
 * @param event
 * Two format are supported : Paparazzi messages, and geo URI
 * Paparazzi messages : "<id>:<class>:<msg name>:<field>:<???>" ex: "2:telemetry:WP_MOVED:utm_north:1."
 * geo URI : "geo:<lat>,<lon>[,<alt>][?z=<zoom>]"
 */
void MapWidget::dropEvent(QDropEvent *event) {
    QString text = event->mimeData()->text();
    QStringList args = text.split(QString(":"));

    QRegularExpression geo_re("^geo:(\\d+\\.\\d+),(-?\\d+\\.\\d+)(?:,-?\\d+\\.?\\d*)?(?:\\?z=(\\d+))?$");
    QRegularExpressionMatch geo_match = geo_re.match(text);

    QRegularExpression pprz_msg_re("^(\\w+):(\\w+):(\\w+):(\\w+):.*$");
    QRegularExpressionMatch pprz_msg_match = pprz_msg_re.match(text);

    if (geo_match.hasMatch()) {
        //geo URI, format : geo:<lat>,<lon>[,<alt>][?z=<zoom>]
        // example: geo:37.786971,-122.399677,120.23?z=12
        double lat = geo_match.captured(1).toDouble();
        double lon = geo_match.captured(2).toDouble();
        centerLatLon(Point2DLatLon(lat, lon));

        if(geo_match.lastCapturedIndex() == 3) {
            int z = geo_match.captured(3).toInt();
            setZoom(z);
        }
    } else if(pprz_msg_match.hasMatch()) {
        // Paparazzi message, ex: "2:telemetry:WP_MOVED:utm_north:1."
        QString ac_id = pprz_msg_match.captured(1);
        // QString msg_class = pprz_msg_match.captured(2);
        QString msg_name = pprz_msg_match.captured(3);
        QString field = pprz_msg_match.captured(4);

        struct Papget::DataDef datadef = {
            ac_id,
            msg_name,
            field,
        };

        Papget* papget = new Papget(datadef, event->pos());
        scene()->addItem(papget);
        papget->setZValue(1000);
        papgets.append(papget);

        connect(papget, &Papget::moved, this, [=](QPointF pos) {
            QPoint viewPos = mapFromScene(pos);
            papget->setPosition(viewPos);
        });

        papget->updateGraphics(this);

    }
}

void MapWidget::handleNewAC(QString ac_id) {
    (void)ac_id;
    auto fp = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan();
    auto wp_orig = fp->getOrigin();
    auto pt = Point2DLatLon(wp_orig->getLat(), wp_orig->getLon());
    //TODO adapt zoom to Flightplan boundinx box
    setZoom(17);
    centerLatLon(pt);
}
