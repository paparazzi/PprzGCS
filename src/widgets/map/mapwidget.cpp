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
#include "item_edit_state_machine.h"
#include "waypoint_item.h"
#include "acitemmanager.h"
#include "circle_item.h"
#include "waypointeditor.h"
#include "windindicator.h"
#include "intruder_item.h"
#include "arrow_item.h"
#include "quiver_item.h"


MapWidget::MapWidget(QWidget *parent) : Map2D(parent),
    interaction_state(PMIS_OTHER), drawState(false), fp_edit_sm(nullptr), gcsItem(nullptr),
    pan_state(PAN_IDLE), pan_mouse_mask(Qt::MiddleButton | Qt::LeftButton),
    _ac_arrow_size(30)
{
    auto settings = getAppSettings();

    horizontalLayout = new QHBoxLayout(this);   // main layout
    buttonsLeftLayout = new QVBoxLayout();
    columnLeft = new QVBoxLayout();
    rightLayout = new QVBoxLayout();
    auto h_right = new QHBoxLayout();
    columnRight = new QVBoxLayout();
    buttonsRightLayout = new QVBoxLayout();

    horizontalLayout->addItem(buttonsLeftLayout);
    horizontalLayout->addItem(columnLeft);
    horizontalLayout->addStretch(1);
    horizontalLayout->addItem(rightLayout);
    rightLayout->addItem(h_right);
    h_right->addItem(columnRight);
    h_right->addItem(buttonsRightLayout);

    buttonsLeftLayout->addStretch(1);
    buttonsRightLayout->addStretch(1);
    columnLeft->addStretch(1);
    columnRight->addStretch(1);

    setDragMode(QGraphicsView::NoDrag);
    setMouseLoadTileMask(Qt::MiddleButton | Qt::LeftButton);

    setMouseTracking(true);

    setZoom(2);
    centerLatLon(Point2DLatLon(43.462344,1.273044));

    for(auto &tp: tileProviders() ) {
        auto config = tp->config();
        if(config->name == settings.value("map/default_tiles").toString()) {
            toggleTileProvider(config->name, true);
        }
    }

    timer_intruders = new QTimer(this);
    connect(timer_intruders, &QTimer::timeout, this, [=]() {
        auto now = QTime::currentTime();
        auto it = QMutableMapIterator(intruders);
        while(it.hasNext()) {
            auto next = it.next();
            if(next.value().second.addSecs(20) < now) {
                removeItem(next.value().first);
                it.remove();
            }
        }
    });
    timer_intruders->start(1000);

    connect(scene(), &MapScene::eventScene, this,
        [=](SmEditEvent eventType, QGraphicsSceneMouseEvent *mouseEvent) {
            if(interaction_state == PMIS_FLIGHT_PLAN_EDIT && fp_edit_sm != nullptr) {
                MapItem* item = fp_edit_sm->update(eventType, mouseEvent, nullptr, current_ac);
                (void)item; //put item in a list relative to the drone (in a drone FP, in a block)
            }
        });


    connect(DispatcherUi::get(), &DispatcherUi::showHiddenWaypoints, this, [=](bool state) {
        for(auto &itemManager: ac_items_managers) {
            for(auto &wpi: itemManager->getWaypointsItems()) {
                if(state) {
                    wpi->setStyle(GraphicsObject::Style::DEFAULT);
                } else {
                    if(wpi->getOriginalWaypoint()->getName().startsWith('_')) {
                        wpi->setStyle(GraphicsPoint::Style::CURRENT_NAV);
                    }
                }
            }
        }
    });

    connect(AircraftManager::get(), &AircraftManager::waypoint_changed, this, &MapWidget::onWaypointChanged);
    connect(AircraftManager::get(), &AircraftManager::waypoint_added, this, &MapWidget::onWaypointAdded);
    connect(DispatcherUi::get(), &DispatcherUi::move_waypoint_ui, this, &MapWidget::onMoveWaypointUi);

    connect(  DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &MapWidget::handleNewAC);
    connect(  DispatcherUi::get(), &DispatcherUi::ac_deleted, this, &MapWidget::removeAC);
    connect(  DispatcherUi::get(), &DispatcherUi::ac_selected, this, &MapWidget::changeCurrentAC);
    connect(PprzDispatcher::get(), &PprzDispatcher::flight_param, this, &MapWidget::updateAircraftItem);
    connect(PprzDispatcher::get(), &PprzDispatcher::nav_status, this, &MapWidget::updateTarget);
    connect(PprzDispatcher::get(), &PprzDispatcher::circle_status, this, &MapWidget::updateNavShape);
    connect(PprzDispatcher::get(), &PprzDispatcher::segment_status, this, &MapWidget::updateNavShape);

    shape_bind_id = PprzDispatcher::get()->bind("SHAPE", this,
        [=](QString sender, pprzlink::Message msg) {
            onShape(sender, msg);
        });

    PprzDispatcher::get()->bind("INTRUDER", this,
        [=](QString sender, pprzlink::Message msg) {
            onIntruder(sender, msg);
        });

    PprzDispatcher::get()->bind("QUIVER", this,
        [=](QString sender, pprzlink::Message msg) {
            onQuiver(sender, msg);
        });

    PprzDispatcher::get()->bind("FLIGHT_PARAM", this,
            [=](QString sender, pprzlink::Message msg) {
                (void)sender;
                QString id;
                msg.getField("ac_id", id);
                if(id == "GCS") {
                    onGCSPos(msg);
                }
            });


    setAcceptDrops(true);
}


void MapWidget::setEditorMode() {
    switch(interaction_state) {
        case PMIS_FLIGHT_PLAN_EDIT:
            itemsForbidHighlight(true);
            itemsEditable(false);
            break;
        case PMIS_FROZEN:
            itemsForbidHighlight(false);
            itemsEditable(false);
            break;
        default:
            itemsForbidHighlight(false);
            itemsEditable(true);
            break;
    }
}

void MapWidget::changeCurrentAC(QString id) {
    current_ac = id;
    updateHighlights(id);
}

void MapWidget::registerWaypoint(WaypointItem* waypoint) {
    connect(waypoint, &WaypointItem::itemClicked, this,
        [=](QPointF pos) {
            (void)pos;
            if(interaction_state == PMIS_FLIGHT_PLAN_EDIT && fp_edit_sm != nullptr) {
                MapItem* item = fp_edit_sm->update(FPEE_WP_CLICKED, nullptr, waypoint, current_ac);
                (void)item; //put item in a list relative to the drone (in a drone FP, in a block)
            }
        });
}

LayerCombo* MapWidget::makeLayerCombo() {
    auto settings = getAppSettings();

    auto layer_combo = new LayerCombo(this);
    //layer_combo->setStyleSheet("QWidget{background-color: #31363b;} QLabel{color:white;}");

    auto tps = tileProviders();
    for(auto &tp: tps ) {
        int z = tps.count() - tp->config()->initial_rank;
        bool visible = tp->isVisible();
        layer_combo->makeLayerControl(tp->config()->name, visible, z);
    }

    connect(
        layer_combo, &LayerCombo::showLayer, this,
        [=](QString name, bool state) {
            toggleTileProvider(name, state);
            updateTiles();
        }
    );

    connect(
        layer_combo, &LayerCombo::layerOpacityChanged, this,
        [=](QString name, qreal opacity) {
            setLayerOpacity(name, opacity);
        }
    );

    connect(
        layer_combo, &LayerCombo::zValueChanged, this,
        [=](QString name, int z) {
            setLayerZ(name, z);
        }
    );

    return layer_combo;

    //auto button = new LockButton(QIcon(settings.value("APP_DATA_PATH").toString() + "/pictures/" + "map_layers_normal.svg"), this);

    //addWidget(layer_combo, button, WIDGETS_LEFT);
}

void MapWidget::addWidget(QWidget* widget, LockButton* button, WidgetContainer side) {
    QList<LockButton*>* buttons;
    if(side == WIDGETS_LEFT) {
        columnLeft->insertWidget(columnLeft->count() - 1, widget);
        buttonsLeftLayout->insertWidget(buttonsLeftLayout->count()-1, button);
        button->setActiveSide(false);
        buttons = &buttonsLeft;
    } else  {
        columnRight->insertWidget(columnRight->count() - 1, widget, 0, Qt::AlignRight);
        buttonsRightLayout->insertWidget(buttonsRightLayout->count()-1, button, 0, Qt::AlignRight);
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
    for(auto column_ele=ele.firstChildElement(); !column_ele.isNull(); column_ele=column_ele.nextSiblingElement()) {
        for(auto widget_ele=column_ele.firstChildElement(); !widget_ele.isNull(); widget_ele=widget_ele.nextSiblingElement()) {
            assert(widget_ele.tagName() == "widget");
            auto name = widget_ele.attribute("name");
            auto container = widget_ele.attribute("container", "stack");

            QWidget* widget = nullptr;
            if(name == "layers") {
                widget = makeLayerCombo();
            } else {
                widget = makeWidget(this, container, name);
            }

            auto conf = widget_ele.firstChildElement("configure");
            if(!conf.isNull()) {
                auto c = dynamic_cast<Configurable*>(widget);
                if(c) {
                    c->configure(conf);
                }
            }

            WidgetContainer side;
            if(column_ele.tagName() == "columnLeft") {
                side = WIDGETS_LEFT;
            } else if (column_ele.tagName() == "columnRight") {
                side = WIDGETS_RIGHT;
            } else {
                throw runtime_error("Unknow tag" + column_ele.tagName().toStdString());
            }

            auto icon_src = widget_ele.attribute("icon");
            if(icon_src == "") {
                std::string s = "Missing attribute 'icon' for node " + column_ele.tagName().toStdString() + "!";
                throw runtime_error(s);
            }

            auto icon = QIcon(user_or_app_path("pictures/" + icon_src));
            auto button = new LockButton(icon, this);
            button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

            addWidget(widget, button, side);
            installEventFilter(widget);

        }
    }

    wind_indicator = new WindIndicator(this);
    wind_indicator->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    rightLayout->addWidget(wind_indicator, 0, Qt::AlignRight);

    PprzDispatcher::get()->bind("WIND", this, [=](QString sender, pprzlink::Message msg){
        (void)sender;
        QString ac_id;
        msg.getField("ac_id", ac_id);
        double dir = getFloatingField(msg, "dir");
        double wspeed = getFloatingField(msg, "wspeed");
        // double mean_aspeed = getFloatingField(msg, "mean_aspeed");
        // double stddev = getFloatingField(msg, "stddev");
        wind_indicator->setWindData(ac_id, dir, wspeed);
    });

    connect(wind_indicator, &WindIndicator::requestRotation, this, [=](double rot) {
        rotateMap(rot-getRotation());
    });

}

void MapWidget::setCursor(const QCursor &cur) {
    QGraphicsView::setCursor(cur);
//    leftScrollArea->setCursor(Qt::ArrowCursor);
//    layers_button->setCursor(Qt::ArrowCursor);
}

void MapWidget::addItem(MapItem* map_item) {
    map_item->addToMap(this);
    map_item->updateGraphics(this, UpdateEvent::ANY);
    _items.append(map_item);
    emit itemAdded(map_item);

    map_item->setHighlighted(map_item->acId() == current_ac || map_item->acId() == "__NO_AC__");

    WaypointItem* wi = dynamic_cast<WaypointItem*>(map_item);
    if(wi != nullptr) {
        registerWaypoint(wi);
    }

    connect(map_item, &MapItem::itemChanged, map_item, [=]() {
        map_item->updateGraphics(this, UpdateEvent::ITEM_CHANGED);
    });

    connect(map_item, &MapItem::itemGainedHighlight, map_item, [=]() {
        QString ac_id = map_item->acId();
        if(ac_id != "__NO_AC__") {
            emit DispatcherUi::get()->ac_selected(ac_id);
        }
    });
}

void MapWidget::removeItem(MapItem* item) {
    item->removeFromScene(this);
    _items.removeAll(item);
    emit itemRemoved(item);
    item->deleteLater();
}

void MapWidget::itemsForbidHighlight(bool fh) {
    for(auto item: qAsConst(_items)) {
        item->setForbidHighlight(fh);
    }
}

void MapWidget::itemsEditable(bool ed) {
    for(auto item: qAsConst(_items)) {
        item->setEditable(ed);
    }
}

void MapWidget::updateHighlights(QString ac_id) {
    for(auto item: qAsConst(_items)) {
        if(item->acId() == ac_id || item->acId() == "__NO_AC__") {
            item->setHighlighted(true);
        } else {
            item->setHighlighted(false);
        }
    }
}

void MapWidget::centerLatLon(Point2DLatLon latLon) {
    Map2D::centerLatLon(latLon);
    updateGraphics(UpdateEvent::MAP_MOVED);
}

void MapWidget::setZoom(double z) {
    Map2D::setZoom(z);
    updateGraphics(UpdateEvent::MAP_ZOOMED);
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
            auto tr = transform();
            double scale2 = tr.m11()*tr.m11() + tr.m12()*tr.m12();
            auto dx = (dp.x() * tr.m11() + dp.y() * tr.m12()) / scale2;
            auto dy = (dp.x() * tr.m21() + dp.y() * tr.m22()) / scale2;
            translate(dx, dy);
            lastPos = event->pos();
        }

        updateGraphics(UpdateEvent::MOUSE_MOVED);
    }
    emit mouseMoved(mapToScene(event->pos()));
}

void MapWidget::mouseReleaseEvent(QMouseEvent *event) {
    Map2D::mouseReleaseEvent(event);
    if(current_ac != "") {
        auto ac = AircraftManager::get()->getAircraft(current_ac);
        if(pan_state != PAN_MOVE && !ac->isReal()) {
            auto modifiers = qApp->keyboardModifiers();
            if(modifiers.testFlag(Qt::KeyboardModifier::ControlModifier)) {
                auto fp = AircraftManager::get()->getAircraft(current_ac)->getFlightPlan();
                QPointF scenePos = mapToScene(event->pos());
                auto pos = CoordinatesTransform::get()->wgs84_from_scene(scenePos, zoomLevel(zoom()), tileSize());
                auto wp = fp->addWaypoint("???", pos);
                emit AircraftManager::get()->waypoint_added(wp, current_ac);
            }
        }
    }
    pan_state = PAN_IDLE;
}


void MapWidget::keyReleaseEvent(QKeyEvent *event) {
    (void)event;
    if(event->key() == Qt::Key_Space) {
        interaction_state = PMIS_FLIGHT_PLAN_EDIT;
        setEditorMode();
        switch (drawState) {
        default:
            break;
        }
        drawState = (drawState + 1) % 3;
    }
    else if(event->key() == Qt::Key_Escape) {
        if(interaction_state == PMIS_FLIGHT_PLAN_EDIT && fp_edit_sm != nullptr) {
            MapItem* item = fp_edit_sm->update(FPEE_CANCEL, nullptr, nullptr, current_ac);
            (void)item; //put item in a list relative to the drone (in a drone FP, in a block)
        }
        setMouseTracking(false);
        scene()->setShortcutItems(false);
        interaction_state = PMIS_OTHER;
        setEditorMode();
        drawState = 0;
        setCursor(Qt::ArrowCursor);
    } else if(event->key() == Qt::Key_F) {
        interaction_state = PMIS_FROZEN;
        setEditorMode();
    }
    else if (event->key() == Qt::Key_H) {
        itemsForbidHighlight(false);
    }
    else if (event->key() == Qt::Key_C) {
        if(AircraftManager::get()->aircraftExists(current_ac)) {
            auto ac = AircraftManager::get()->getAircraft(current_ac);
            auto orig = ac->getFlightPlan()->getOrigin();
            Point2DLatLon pos(orig);
            if(ac->isReal()) {
                pos = ac->getPosition();
            }
            centerLatLon(pos);
        }
        itemsForbidHighlight(false);
    }
    else if (event->key() == Qt::Key_R) {
        double rotation = 10;
        if(event->modifiers() & Qt::KeyboardModifier::AltModifier) {
            rotation = -getRotation();
        } else {
            if(event->modifiers() & Qt::KeyboardModifier::ControlModifier) {
                rotation = -rotation;
            }
            if(event->modifiers() & Qt::KeyboardModifier::ShiftModifier) {
                rotation /= 2;
            }
        }
        rotateMap(rotation);
    }
}

void MapWidget::rotateMap(double rot) {
    auto center = mapToScene(rect().center());
    rotate(rot);
    centerOn(center);
    updateGraphics(UpdateEvent::MAP_ROTATED);
    wind_indicator->setCompass(getRotation());
}


bool MapWidget::viewportEvent(QEvent *event) {
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();

        for(auto &touchPoint: touchPoints) {
            if(touchPoint.state() == Qt::TouchPointPressed) {
                auto scenePos = mapToScene(touchPoint.pos().toPoint());
                auto tp = tilePoint(scenePos, zoomLevel(zoom()), tileSize());
                pms[touchPoint.id()] = Point2DPseudoMercator(tp);
            }
            else if(touchPoint.state() == Qt::TouchPointReleased && pms.contains(touchPoint.id())) {
                pms.remove(touchPoint.id());
            }
        }

        if(touchPoints.count() == 2 && !(touchEvent->touchPointStates() & Qt::TouchPointPressed)) {
            //Zoom and pan combined
            auto id0 = pms.firstKey();
            auto id1 = pms.lastKey();

            auto tp0 = std::find_if(touchPoints.begin(), touchPoints.end(), [=](auto ele) {return ele.id() == id0;});
            auto tp1 = std::find_if(touchPoints.begin(), touchPoints.end(), [=](auto ele) {return ele.id() == id1;});

            if(tp0 == touchPoints.end() || tp1 == touchPoints.end()) {
                // touch points not found!
                qDebug() << "a touch point was not found!" << pms.count();
                return true;
            }

            auto px_dist = QVector2D(tp1->pos() - tp0->pos()).length();

            for(int zo=0; zo<25; zo++) {
                auto pt1 = scenePoint(pms[id0], zo, tileSize());
                auto pt2 = scenePoint(pms[id1], zo, tileSize());
                auto dist = QVector2D(pt2-pt1).length();
                if(dist > px_dist) {
                    // found the right zoom level
                    double s = px_dist/dist;
                    double new_zoom = zo + log2(s);

                    auto center = (tp1->pos() + tp0->pos())/2;
                    auto pmc = (pms[id0] + pms[id1])/2;
                    zoomCenteredScene(new_zoom, center.toPoint(), pmc);
                    updateGraphics(UpdateEvent::MAP_ZOOMED|UpdateEvent::MAP_MOVED|UpdateEvent::MAP_ROTATED);
                    break;
                }
            }
            return true;
        } else if(touchPoints.count() == 1 && touchPoints.first().state() == Qt::TouchPointMoved) {
            //Pan only
            auto pm = pms[touchPoints.first().id()];
            auto pos = touchPoints.first().pos().toPoint();
            zoomCenteredScene(zoom(), pos, pm);
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QGraphicsView::viewportEvent(event);
}

void MapWidget::updateGraphics(uint32_t update_events) {
    for(auto item: qAsConst(_items)) {
        item->updateGraphics(this, update_events);
    }
    for(auto papget: qAsConst(papgets)) {
        papget->updateGraphics(this);
    }
}

void MapWidget::wheelEvent(QWheelEvent* event) {
    Map2D::wheelEvent(event);
    updateGraphics(UpdateEvent::MAP_ZOOMED);
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

    auto ac = AircraftManager::get()->getAircraft(ac_id);

    auto settings = getAppSettings();

    ACItemManager* item_manager;
    if(ac->isReal()) {
        // create aircraft item at dummy position
        auto aircraft_item = new AircraftItem(Point2DLatLon(0, 0), ac_id, 16);
        addItem(aircraft_item);

        //create carrot at dummy position
        WaypointItem* target = new WaypointItem(Point2DLatLon(0, 0), ac_id);
        addItem(target);
        target->setStyle(GraphicsObject::Style::CARROT);
        target->setEditable(false);
        double z_carrot = settings.value("map/z_values/carrot").toDouble();
        target->setZValues(z_carrot, z_carrot);

        ArrowItem* arrow = new ArrowItem(ac_id, 15, this);
        addItem(arrow);
        arrow->setProperty("size", _ac_arrow_size);

        connect(arrow, &ArrowItem::centerAC, this, [=]() {
            auto pos = ac->getPosition();
            centerLatLon(pos);
        });

        //create the ACItemManager for this aircraft
        item_manager = new ACItemManager(ac_id, target, aircraft_item, arrow, this);
    } else {
        //create the ACItemManager for this fake aircraft (flightplan only)
        item_manager = new ACItemManager(ac_id, nullptr, nullptr, nullptr, this);
    }

    ac_items_managers[ac_id] = item_manager;

    auto fp = ac->getFlightPlan();

    for(auto wp: fp->getWaypoints()) {
        onWaypointAdded(wp, ac_id);
    }

    for(auto sector: fp->getSectors()) {
        // static sector are not supported
        PathItem* pi = new PathItem(ac_id, sector->getColor());
        for(auto &wp: sector->getCorners()) {
            for(auto wpi: item_manager->getWaypointsItems()) {
                if(wpi->getOriginalWaypoint() == wp) {
                    pi->addPoint(wpi);
                }
            }
        }

        pi->setClosedPath(true);
        addItem(pi);
        item_manager->addPathItem(pi);
    }

    auto wp_orig = fp->getOrigin();
    auto pt = Point2DLatLon(wp_orig->getLat(), wp_orig->getLon());
    //TODO adapt zoom to Flightplan boundinx box
    setZoom(17);
    centerLatLon(pt);
}

void MapWidget::removeAC(QString ac_id) {
    ac_items_managers[ac_id]->removeItems(this);
    ac_items_managers[ac_id]->deleteLater();
    ac_items_managers.remove(ac_id);
}

void MapWidget::onWaypointChanged(Waypoint* wp, QString ac_id) {
    if(AircraftManager::get()->aircraftExists(ac_id)) {
        for(auto wpi: ac_items_managers[ac_id]->getWaypointsItems()) {
            if(wpi->getOriginalWaypoint() == wp && !wpi->isMoving()) {
                wpi->update();
                wpi->setAnimate(false);
            }
        }
    }
}

void MapWidget::onWaypointAdded(Waypoint* wp, QString ac_id) {
    WaypointItem* wpi = new WaypointItem(wp, ac_id);
    addItem(wpi);
    ac_items_managers[ac_id]->addWaypointItem(wpi);

    auto dialog_move_waypoint = [=]() {
        wpi->setMoving(true);
        auto we = new WaypointEditor(wpi, ac_id);
        auto view_pos = mapFromScene(wpi->scenePos());
        auto global_pos = mapToGlobal(view_pos);
        we->show(); //show just to get the width and height right.
        we->move(global_pos - QPoint(we->width()/2, we->height()/2));
        connect(we, &QDialog::finished, wpi, [=](int result) {
            (void)result;
            wpi->setMoving(false);
            if(!result) {
                wpi->update();
                wpi->setAnimate(false);
            }
        });
        we->open();
    };

    connect(wpi, &WaypointItem::waypointMoveFinished, this, dialog_move_waypoint);

    connect(wpi, &WaypointItem::itemDoubleClicked, this, dialog_move_waypoint);

    connect(wpi, &WaypointItem::waypointMoved, this, [=](){
        wpi->setAnimate(true);
    });

    if(wp->getName().startsWith('_')) {
        wpi->setStyle(GraphicsPoint::Style::CURRENT_NAV);
    }

    if(wp->getName() == "HOME") {
        auto fp = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan();
        auto ci = new CircleItem(wpi, fp->getMaxDistFromHome(), ac_id);
        ci->setEditable(false);
        addItem(ci);
        ac_items_managers[ac_id]->setMaxDistCircle(ci);
    }

}

void MapWidget::onMoveWaypointUi(Waypoint* wp, QString ac_id) {
    // If this is a "flight plan only" AC, move the original waypoint
    if(!AircraftManager::get()->getAircraft(ac_id)->isReal()) {
        for(auto wpi: ac_items_managers[ac_id]->getWaypointsItems()) {
            if(wpi->waypoint() == wp) {
                wpi->commitPosition();
                wpi->setAnimate(false);
                emit AircraftManager::get()->waypoint_changed(wpi->getOriginalWaypoint(), ac_id);
            }
        }
    }
}

void MapWidget::updateTarget(pprzlink::Message msg) {
    QString ac_id;
    msg.getField("ac_id", ac_id);
    double target_lat = getFloatingField(msg, "target_lat");
    double target_lon = getFloatingField(msg, "target_long");

    if(AircraftManager::get()->aircraftExists(ac_id)) {
        ac_items_managers[ac_id]->getTarget()->setPosition(Point2DLatLon(target_lat, target_lon));
    }
}

void MapWidget::updateNavShape(pprzlink::Message msg) {
    auto settings = getAppSettings();

    QString ac_id;
    msg.getField("ac_id", ac_id);

    MapItem* prev_item = ac_items_managers[ac_id]->getCurrentNavShape();

    auto delete_prev = [=]() {
        if(prev_item != nullptr) {
            // prev_item not null, delete it
            ac_items_managers[ac_id]->setCurrentNavShape(nullptr);
            removeItem(prev_item);
        }
    };

    double z = settings.value("map/z_values/nav_shape").toDouble();

    if(msg.getDefinition().getName() == "CIRCLE_STATUS") {
        int16_t radius;
        double circle_lat = getFloatingField(msg, "circle_lat");
        double circle_long = getFloatingField(msg, "circle_long");
        msg.getField("radius", radius);
        Point2DLatLon pos(circle_lat, circle_long);

        CircleItem* ci = dynamic_cast<CircleItem*>(prev_item);

        if(ci == nullptr) {
            delete_prev();
            auto wcenter = new WaypointItem(pos, ac_id);
            wcenter->setZValues(z, z);
            addItem(wcenter);
            ci = new CircleItem(wcenter, radius, ac_id);
            ci->setZValues(z, z);
            ci->setScalable(false);
            ci->setEditable(false);
            ci->setOwnCenter(true);
            addItem(ci);
            ci->setStyle(GraphicsObject::Style::CURRENT_NAV);
            ac_items_managers[ac_id]->setCurrentNavShape(ci);
        } else {
            ci->getCenter()->getOriginalWaypoint()->setLat(pos.lat());
            ci->getCenter()->getOriginalWaypoint()->setLon(pos.lon());
            ci->getCenter()->update();
            ci->setRadius(radius);
        }

    } else if (msg.getDefinition().getName() == "SEGMENT_STATUS") {
        double segment1_lat = getFloatingField(msg, "segment1_lat");
        double segment1_long = getFloatingField(msg, "segment1_long");
        double segment2_lat = getFloatingField(msg, "segment2_lat");
        double segment2_long = getFloatingField(msg, "segment2_long");

        Point2DLatLon p1(segment1_lat, segment1_long);
        Point2DLatLon p2(segment2_lat, segment2_long);

        PathItem* pi = dynamic_cast<PathItem*>(prev_item);

        if(pi == nullptr) {
            delete_prev();

            pi = new PathItem(ac_id);
            pi->setZValues(z, z);

            auto w1 = new WaypointItem(p1, ac_id);
            w1->setZValues(z, z);
            addItem(w1);
            auto w2 = new WaypointItem(p2, ac_id);
            w2->setZValues(z, z);
            addItem(w2);
            pi->addPoint(w1);
            pi->addPoint(w2);
            pi->setStyle(GraphicsObject::Style::CURRENT_NAV);
            addItem(pi);
            ac_items_managers[ac_id]->setCurrentNavShape(pi);
            //qDebug() << "segment created!";
        } else {
            auto wps = pi->getWaypoints();
            assert(wps.size() == 2);
            wps[0]->getOriginalWaypoint()->setLat(p1.lat());
            wps[0]->getOriginalWaypoint()->setLon(p1.lon());
            wps[0]->update();

            wps[1]->getOriginalWaypoint()->setLat(p2.lat());
            wps[1]->getOriginalWaypoint()->setLon(p2.lon());
            wps[1]->update();
        }

    }


}


void MapWidget::updateAircraftItem(pprzlink::Message msg) {
    QString ac_id;
    msg.getField("ac_id", ac_id);
    double lat = getFloatingField(msg, "lat");
    double lon = getFloatingField(msg, "long");
    double heading = getFloatingField(msg, "heading");

    if(AircraftManager::get()->aircraftExists(ac_id)) {
        auto ai = ac_items_managers[ac_id]->getAircraftItem();
        auto arrow = ac_items_managers[ac_id]->getArrowItem();
        Point2DLatLon pos(static_cast<double>(lat), static_cast<double>(lon));
        ai->setPosition(pos);
        arrow->setAcPos(pos);
        ai->setHeading(static_cast<double>(heading));
        AircraftManager::get()->getAircraft(ac_id)->setPosition(pos);

        if(GlobalConfig::get()->value("MAP_TRACK_AC", false).toBool() && ac_id == current_ac) {
            auto scenePos = scenePoint(pos, zoomLevel(zoom()), tileSize());
            centerOn(scenePos);
            rotateMap(-heading - getRotation());
        }

    }

}


void MapWidget::onShape(QString sender, pprzlink::Message msg) {
    (void)sender;
    /////////////////////////////////////////////////////////
    /// Get shape_id and status: create/delete
    uint8_t id;
    uint8_t shape, status;
    msg.getField("id", id);
    msg.getField("shape", shape);
    msg.getField("status", status);

    QString shape_id;
    if(shape == 0) { //Circle
        shape_id = "Circle_" + QString::number(id);
    } else if(shape == 1) {
        shape_id = "Polygon_" + QString::number(id);
    } else if(shape == 2) {
        shape_id = "Line_" + QString::number(id);
    } else {
        qDebug() << "Unknow shape.";
        return;
    }

    /// in all cases: delete the shape. Then re-create it if it was just an update
    if(shapes.contains(shape_id)) {
        auto item = shapes[shape_id];
        shapes.remove(shape_id);
        removeItem(item);
    }

    if(status == 1) { //deletion, return now.
        return;
    }

    /////////////////////////////////////////////////////////
    /// build Palette
    QString linecolor, fillcolor;
    uint8_t opacity;
    msg.getField("linecolor", linecolor);
    msg.getField("fillcolor", fillcolor);
    msg.getField("opacity", opacity);
    int alpha;
    switch(opacity) {
        case 0: alpha = 0; break;
        case 1: alpha = 85; break;
        case 2: alpha = 170; break;
        case 3: alpha = 255; break;
        default: alpha = 255;
    }
    auto line = QColor(linecolor);
    auto fill = QColor(fillcolor);
    fill.setAlpha(alpha);
    auto palette = PprzPalette(line, fill);

    QList<int32_t> latarr;
    QList<int32_t> lonarr;
    float radius;
    QString text;
    msg.getField("latarr", latarr);
    msg.getField("lonarr", lonarr);
    msg.getField("radius", radius);
    msg.getField("text", text);

    if(latarr.size() != lonarr.size()) {
        qDebug() << "latarr and lonarr have different size. Abort.";
        return;
    }

    QVector<Point2DLatLon> points;
    for(int i=0; i<latarr.size(); ++i) {
        points.append(Point2DLatLon(latarr[i]/1e7, lonarr[i]/1e7));
    }


    auto settings = getAppSettings();
    double z = settings.value("map/z_values/shapes").toDouble();

    MapItem* item = nullptr;

    if(shape == 0) { //Circle
        if(points.size() < 1) {
            qDebug() << "Circle shape need one point!";
            return;
        }

        auto pos = points[0];
        auto wcenter = new WaypointItem(pos, "__NO_AC__", palette);
        //wcenter->setEditable(false);
        //wcenter->setZValues(z, z);
        addItem(wcenter);
        wcenter->setStyle(GraphicsObject::Style::CURRENT_NAV);

        CircleItem* ci = new CircleItem(wcenter, radius, "__NO_AC__", palette);
        wcenter->setParent(ci);

        ci->setOwnCenter(true);
        ci->setZValues(z, z);
        ci->setScalable(false);
        ci->setEditable(false);
        ci->setOwnCenter(true);
        ci->setFilled(true);
        ci->setText(text);
        addItem(ci);
        item = ci;

    } else if(shape == 1 || shape == 2) { // Polygon || Line
        if(points.size() < 2) {
            qDebug() << "Line/Polygon shape need at least two points!";
            return;
        }
        auto pi = new PathItem("__NO_AC__", palette);
        if(shape == 1) {    // Polygon
            pi->setFilled(true);
            pi->setClosedPath(true);
        }
        pi->setZValues(z, z);
        for(auto pos: points) {
            auto wi = new WaypointItem(pos, "__NO_AC__", palette);
            //wcenter->setEditable(false);
            //wcenter->setZValues(z, z);
            addItem(wi);
            wi->setStyle(GraphicsObject::Style::CURRENT_NAV);
            pi->addPoint(wi);
        }
        pi->setText(text);
        addItem(pi);
        item = pi;

    }

    if(item != nullptr && !shape_id.isNull()) {
        shapes[shape_id] = item;
    }

}

void MapWidget::onIntruder(QString sender, pprzlink::Message msg) {
    (void)sender;
    QString id, name;
    int32_t lat, lon;   // alt;
    float course;   // speed, climb;
    //uint32_t itow;

    msg.getField("id", id);
    msg.getField("name", name);
    msg.getField("lat", lat);
    msg.getField("lon", lon);
    //msg.getField("alt", alt);
    msg.getField("course", course);
    //msg.getField("speed", speed);
    //msg.getField("climb", climb);
    //msg.getField("itow", itow);

    if(intruders.contains(id)) {
        removeItem(intruders[id].first);
        intruders.remove(id);
    }

    auto pos = Point2DLatLon(lat/1e7, lon/1e7);

    auto itd = new IntruderItem(name, pos, course);
    addItem(itd);

    intruders[id] = make_pair(itd, QTime::currentTime());
}

void MapWidget::onQuiver(QString sender, pprzlink::Message msg) {
    (void)sender;
    QString id;
    uint8_t status;
    int32_t lat, lon;
    int32_t vlat, vlon;

    msg.getField("id", id);
    msg.getField("status", status);
    msg.getField("lat", lat);
    msg.getField("lon", lon);
    msg.getField("vlat", vlat);
    msg.getField("vlon", vlon);

    if(id == "clean") {
        for (MapItem* quiver : quivers){
            removeItem(quiver);
        }
        quivers.clear();
        return;
    }

    if(quivers.contains(id)) {
        removeItem(quivers[id]);
        quivers.remove(id);
    }

    if(status == 1) { //deletion, return now.
        return;
    }

    auto pos = Point2DLatLon(lat/1e7, lon/1e7);
    auto vec = Point2DLatLon(vlat/1e7, vlon/1e7);

    auto quiver = new QuiverItem(pos, vec);
    addItem(quiver);

    quivers[id] = quiver;
}


void MapWidget::onGCSPos(pprzlink::Message msg) {
    if(gcsItem) {
        removeItem(gcsItem);
    }

    double lat = getFloatingField(msg, "lat");
    double lon = getFloatingField(msg, "long");
    auto settings = getAppSettings();
    QColor color(settings.value("map/gcs_icon_color").toString());

    auto wcenter = new WaypointItem(Point2DLatLon(lat, lon), "__NO_AC__", PprzPalette(color));
    wcenter->setEditable(false);
    //wcenter->setZValues(z, z);
    addItem(wcenter);
    wcenter->setStyle(GraphicsObject::Style::GCS);

    int size = settings.value("map/gcs_icon_size").toInt();
    wcenter->setSize(size);
    gcsItem = wcenter;
}

void MapWidget::setAcArrowSize(int s) {
    _ac_arrow_size = s;
    for(auto item_manager: ac_items_managers) {
        auto arrow = item_manager->getArrowItem();
        if(arrow) {
            arrow->setProperty("size", s);
        }
    }
}

