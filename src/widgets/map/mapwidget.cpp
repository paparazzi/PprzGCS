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
#include "pprzmain.h"
#include "coordinatestransform.h"

#include "quiver_item.h"
#include "gvf_traj_line.h"
#include "gvf_traj_ellipse.h"
#include "gvf_traj_sin.h"
#include "gvf_traj_trefoil.h"
#include "gvf_traj_3D_ellipse.h"
#include "gvf_traj_3D_lissajous.h"
#include "gvf_traj_bezier.h"

#include "map_items/grid_item.h"


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

    columnLeft->setAlignment(Qt::AlignTop);
    columnRight->setAlignment(Qt::AlignTop);
    buttonsLeftLayout->setAlignment(Qt::AlignTop);
    buttonsRightLayout->setAlignment(Qt::AlignTop);

    horizontalLayout->addItem(buttonsLeftLayout);
    horizontalLayout->addItem(columnLeft);
    horizontalLayout->addStretch(1);
    horizontalLayout->addItem(rightLayout);
    rightLayout->addItem(h_right);
    h_right->addItem(columnRight);
    h_right->addItem(buttonsRightLayout);

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


    // Context menu
    mapMenu = new QMenu("Map", pprzApp()->mainWindow());

    auto copy_coordinates = mapMenu->addAction("Copy coordinates");
    connect(copy_coordinates, &QAction::triggered, this, [=](){
        auto mc = getMouseCoords();
        auto clipboard = QApplication::clipboard();
        auto mc_txt = QString("%1, %2").arg(mc.lat(), 0, 'f', 7).arg(mc.lon(), 0, 'f', 7);
        clipboard->setText(mc_txt);
    });

    show_hidden_wp_action = mapMenu->addAction("Show hidden waypoints");
    show_hidden_wp_action->setCheckable(true);
    connect(show_hidden_wp_action, &QAction::toggled, [=](bool show) {
        setProperty("show_hidden_waypoints", show);
    });

    show_crash_prediction_action = mapMenu->addAction("Show crash prediction");
    show_crash_prediction_action->setCheckable(true);
    connect(show_crash_prediction_action, &QAction::toggled, [=](bool show) {
        setProperty("show_crash_prediction", show);
    });

    auto clear_shapes = mapMenu->addAction("Clear Shapes");
    connect(clear_shapes, &QAction::triggered, this, [=](){
        clearShapes();
    });

    auto clear_dcshots = mapMenu->addAction("Clear DCSHOTS");
    connect(clear_dcshots, &QAction::triggered, this, [=](){
        clearDcShots();
    });

    menu_clear_track = new QMenu("Clear Track", mapMenu);
    mapMenu->addMenu(menu_clear_track);

    // Shortkeys
    auto rotate_map = new QAction(this);
    rotate_map->setShortcut(Qt::Key_R);
    connect(rotate_map, &QAction::triggered, this, [=](){
        rotateMap(10);
    });
    this->addAction(rotate_map);

    auto rotate_map_alt = new QAction(this);
    rotate_map_alt->setShortcut(QKeySequence(Qt::Key_R | Qt::ALT));
    connect(rotate_map_alt, &QAction::triggered, this, [=](){
        rotateMap(-getRotation());
    });
    this->addAction(rotate_map_alt);

    auto rotate_map_ctrl = new QAction(this);
    rotate_map_ctrl->setShortcut(QKeySequence(Qt::Key_R | Qt::CTRL));
    connect(rotate_map_ctrl, &QAction::triggered, this, [=](){
        rotateMap(-10);
    });
    this->addAction(rotate_map_ctrl);

    auto rotate_map_shift = new QAction(this);
    rotate_map_shift->setShortcut(QKeySequence(Qt::Key_R + Qt::SHIFT));
    connect(rotate_map_shift, &QAction::triggered, this, [=](){
        rotateMap(5);
    });
    this->addAction(rotate_map_shift);

    auto rotate_map_ctrl_shift = new QAction(this);
    rotate_map_ctrl_shift->setShortcut(QKeySequence(Qt::Key_R + Qt::CTRL + Qt::SHIFT));
    connect(rotate_map_ctrl_shift, &QAction::triggered, this, [=](){
        rotateMap(-5);
    });
    this->addAction(rotate_map_ctrl_shift);

    auto flight_plan_edit = new QAction(this);
    flight_plan_edit->setShortcut(Qt::Key_Space);
    connect(flight_plan_edit, &QAction::triggered, this, [=](){
        interaction_state = PMIS_FLIGHT_PLAN_EDIT;
        setEditorMode();
        switch (drawState) {
        default:
            break;
        }
        drawState = (drawState + 1) % 3;
    });
    this->addAction(flight_plan_edit);

    auto freeze = new QAction(this);
    freeze->setShortcut(Qt::Key_F);
    connect(freeze, &QAction::triggered, this, [=](){
        interaction_state = PMIS_FROZEN;
        setEditorMode();
    });
    this->addAction(freeze);

    auto center = new QAction(this);
    center->setShortcut(Qt::Key_C);
    connect(center, &QAction::triggered, this, [=](){
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
    });
    this->addAction(center);

    auto highlight = new QAction(this);
    highlight->setShortcut(Qt::Key_H);
    connect(highlight, &QAction::triggered, this, [=](){
        itemsForbidHighlight(false);
    });
    this->addAction(highlight);

    auto escape = new QAction(this);
    escape->setShortcut(Qt::Key_Escape);
    connect(escape, &QAction::triggered, this, [=](){
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
    });
    this->addAction(escape);

    // Connect signals
    connect(AircraftManager::get(), &AircraftManager::waypoint_changed, this, &MapWidget::onWaypointChanged);
    connect(AircraftManager::get(), &AircraftManager::waypoint_added, this, &MapWidget::onWaypointAdded);
    connect(DispatcherUi::get(), &DispatcherUi::move_waypoint_ui, this, &MapWidget::onMoveWaypointUi);
    connect(DispatcherUi::get(), &DispatcherUi::slamGridVisibilityChanged, this,
        [=](bool visible) {
            if(grid_item) {
                grid_item->setVisible(visible);
                
            }
        });
    connect(DispatcherUi::get(), &DispatcherUi::obstacleVisibilityChanged, this,
        [=](bool visible) {
            obstacles_visible = visible;
            if(!visible) {
                for(auto& pair : slam_obstacles) {
                    removeItem(pair.first);
                    delete pair.first;
                }
                slam_obstacles.clear();
            }
        });
    connect(DispatcherUi::get(), &DispatcherUi::gvf_settingUpdated, this,
        [=](QString sender, QVector<int>* gvfViewer_config) {
            gvf_trajectories_config.remove(sender);
            gvf_trajectories_config[sender] = gvfViewer_config;
        });

    connect(  DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &MapWidget::handleNewAC);
    connect(  DispatcherUi::get(), &DispatcherUi::ac_deleted, this, &MapWidget::removeAC);
    connect(  DispatcherUi::get(), &DispatcherUi::ac_selected, this, &MapWidget::changeCurrentAC);
    connect(  DispatcherUi::get(), &DispatcherUi::centerMap, this, &MapWidget::centerLatLon);
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

    PprzDispatcher::get()->bind("DC_SHOT", this,
        [=](QString sender, pprzlink::Message msg) {
            onDcShot(sender, msg);
        });

    PprzDispatcher::get()->bind("FLIGHT_PARAM", this,
        [=](QString sender, pprzlink::Message msg) {
            (void)sender;
            QString id;
            msg.getField("ac_id", id);
            if(id == "GCS") {
                onGCSPos(msg);
            }

            // Update gvf_trajectory vertor field
            if(gvf_trajectories.contains(id)) {
                removeItem(gvf_trajectories[id]->getVField());
                gvf_trajectories[id]->update_VField();
                addItem(gvf_trajectories[id]->getVField());
            }
        });

    PprzDispatcher::get()->bind("GVF", this,
        [=](QString sender, pprzlink::Message msg) {
            onGVF(sender, msg);
        });

    PprzDispatcher::get()->bind("GVF_PARAMETRIC", this,
        [=](QString sender, pprzlink::Message msg) {
            onGVF(sender, msg);
        });

    PprzDispatcher::get()->bind("ROTORCRAFT_FP", this,
        [=](QString sender, pprzlink::Message msg) {
            onROTORCRAFT_FP(sender, msg);
        });

    PprzDispatcher::get()->bind("SLAM", this,
        [=](QString sender, pprzlink::Message msg) {
            onSLAM(sender, msg);
        });

    PprzDispatcher::get()->bind("GRID_INIT", this,
        [=](QString sender, pprzlink::Message msg) {
            onGridInit(sender, msg);
        });

    PprzDispatcher::get()->bind("GRID_CHANGES", this,
        [=](QString sender, pprzlink::Message msg) {
            onGridChanges(sender, msg);
        });

    setAcceptDrops(true);

    // Add menu to app menu bar.
    connect(pprzApp()->mainWindow(), &PprzMain::ready, this, [=](){
        pprzApp()->mainWindow()->menuBar()->addMenu(mapMenu);
    });
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
        columnLeft->addWidget(widget);
        buttonsLeftLayout->addWidget(button);
        button->setActiveSide(false);
        buttons = &buttonsLeft;
    } else  {
        columnRight->addWidget(widget);
        buttonsRightLayout->addWidget(button);
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
            auto id =widget_ele.attribute("id", name);

            QWidget* widget = nullptr;
            if(name == "layers") {
                widget = makeLayerCombo();
            } else {
                widget = makeWidget(this, container, name);
                widget->setObjectName(id);
                if(name == "gvf_viewer") {
                    gvf_loaded = true;
                }
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
    wind_indicator->setObjectName("wind_indicator");
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
    else if(event->buttons() & Qt::RightButton){
        mapMenu->exec(mapToGlobal(event->pos()));
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

void MapWidget::rotateMap(double rot) {
    auto center = mapToScene(rect().center());
    rotate(rot);
    centerOn(center);
    updateGraphics(UpdateEvent::MAP_ROTATED);
    wind_indicator->setCompass(getRotation());
    emit DispatcherUi::get()->mapRotated(getRotation());
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
    Map2D::dragEnterEvent(event);
    if (event->mimeData()->hasFormat("text/plain")) {
        event->acceptProposedAction();
    }
    event->accept();
}

void MapWidget::dragMoveEvent(QDragMoveEvent *event) {
    Map2D::dragMoveEvent(event);
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

    QRegularExpression pprz_msg_re("^(\\w+):(\\w+):(\\w+):(\\w+):(.*)$");
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
        double scale = pprz_msg_match.captured(5).toDouble();

        struct Papget::DataDef datadef = {
            ac_id,
            msg_name,
            field,
            scale,
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

        // create crash item at dummy position
        auto crash_item = new WaypointItem(Point2DLatLon(0, 0), ac_id, 16);
        crash_item->setStyle(GraphicsObject::Style::CRASH);
        if(!show_crash_prediction_action->isChecked()) {
            crash_item->setVisible(false);
        } else {
            crash_item->setVisible(true);
        }
        addItem(crash_item);

        ArrowItem* arrow = new ArrowItem(ac_id, 15, this);
        addItem(arrow);
        arrow->setProperty("size", _ac_arrow_size);

        connect(arrow, &ArrowItem::centerAC, this, [=]() {
            auto pos = ac->getPosition();
            centerLatLon(pos);
        });

        //create the ACItemManager for this aircraft
        item_manager = new ACItemManager(ac_id, target, aircraft_item, arrow, crash_item, this);

        auto clear_track = new QAction(ac->name(), ac);
        connect(clear_track, &QAction::triggered, aircraft_item, [=](){
            aircraft_item->clearTrack();
        });
        menu_clear_track->addAction(clear_track);

    } else {
        //create the ACItemManager for this fake aircraft (flightplan only)
        item_manager = new ACItemManager(ac_id, nullptr, nullptr, nullptr, nullptr, this);
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

    if(gvf_loaded) {
        gvf_trajectories.remove(ac_id);
    }
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

    if(wp->getName().startsWith('_') && !show_hidden_wp_action->isChecked()) {
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
    uint8_t shape_id;
    uint8_t shape, status;
    msg.getField("id", shape_id);
    msg.getField("shape", shape);
    msg.getField("status", status);

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
        for(auto &pos: points) {
            auto wi = new WaypointItem(pos, "__NO_AC__", palette);
            //wcenter->setEditable(false);
            //wcenter->setZValues(z, z);
            addItem(wi);
            wi->setStyle(GraphicsObject::Style::CURRENT_NAV);
            pi->addPoint(wi, QColor(), true);
        }
        pi->setText(text);
        addItem(pi);
        item = pi;

    }

    if(item != nullptr) {
        shapes[shape_id] = item;
    }

}

void MapWidget::clearShapes() {
    for(auto shape_item: shapes) {
        removeItem(shape_item);
    }
    shapes.clear();
}

void MapWidget::clearDcShots() {
    for(auto dsw: dc_shots) {
        removeItem(dsw);
    }
    dc_shots.clear();
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

void MapWidget::onDcShot(QString sender, pprzlink::Message msg) {
    (void)sender;
    int16_t photo_nr;
    int32_t lat, lon;   // alt;
    msg.getField("photo_nr", photo_nr);
    msg.getField("lat", lat);
    msg.getField("lon", lon);

    auto p = PprzPalette(QColor(Qt::yellow), QBrush(Qt::yellow));
    auto dsw = new WaypointItem(Point2DLatLon(lat/1e7, lon/1e7),"__NO_AC__", p , 15, this);
    dsw->setStyle(GraphicsObject::Style::DCSHOT);
    dsw->setEditable(false);

    addItem(dsw);

    dc_shots.append(dsw);
}

void MapWidget::onROTORCRAFT_FP(QString sender, pprzlink::Message msg) {

    int32_t east, north, up, vnorth, veast, vup;
    msg.getField("east", east);
    msg.getField("north", north);
    msg.getField("up", up);
    msg.getField("vnorth", vnorth);
    msg.getField("veast", veast);
    msg.getField("vup", vup);

  if(AircraftManager::get()->aircraftExists(sender)) {
    ac_items_managers[sender]->getCrashItem();

    float g = -9.81f;

    double vx = veast*0.00000190734;
    double vy = vnorth*0.00000190734;
    double vz = vup*0.00000190734;

    double x = east*0.0039063;
    double y = north*0.0039063;
    double z = up*0.0039063;

    double h = fabs(z); // Should be height above ground, make sure to initialize local frame on ground

    // With h always larger than 0, the sqrt can never give nan
    float time_fall = (-vz - sqrtf(vz*vz -2.f*h*g))/g;

    double x_pos = x + time_fall*vx;
    double y_pos = y + time_fall*vy;

    auto ac = AircraftManager::get()->getAircraft(sender);
    auto orig = ac->getFlightPlan()->getOrigin();
    Point2DLatLon pos(orig);

    Point2DLatLon markerpos = CoordinatesTransform::get()->ltp_to_wgs84(pos,x_pos,y_pos);

    ac_items_managers[sender]->getCrashItem()->setPosition(markerpos);
  }
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

void MapWidget::onGVF(QString sender, pprzlink::Message msg) {

    if(!gvf_loaded) {
        return;
    }

    if(!AircraftManager::get()->aircraftExists(sender)) {
        qDebug() << "GVF: AC with id" << sender << "do not exists.";
        return;
    }

    if(gvf_trajectories.contains(sender)) {
        //TODO: checksum??... if it is the same trayectory do not delete the previous one
        removeItem(gvf_trajectories[sender]->getTraj());
        removeItem(gvf_trajectories[sender]->getVField());

        gvf_trajectories[sender]->purge_trajectory();
        delete gvf_trajectories[sender];

        gvf_trajectories.remove(sender);
        ac_items_managers[sender]->setCurrentGVF(nullptr);
    }

    // Common parser definitions
    uint8_t traj;
    float wb;
    QList<float> param = {0.0};
    int8_t direction;
    GVF_trajectory* gvf_traj;

    // GVF message parser
    if(msg.getDefinition().getName() == "GVF") {
        float error, ke;

        msg.getField("error", error);
        msg.getField("traj", traj);
        msg.getField("s", direction);
        msg.getField("ke", ke);
        msg.getField("p", param);

        switch(traj)
        {
            case 0: {// Straight line
                gvf_traj = new GVF_traj_line(sender, param, direction, ke, gvf_trajectories_config[sender]);
                break;
            }
            case 1: { // Ellipse
                gvf_traj = new GVF_traj_ellipse(sender, param, direction, ke, gvf_trajectories_config[sender]);
                break;
            }
            case 2: { // Sin
                gvf_traj = new GVF_traj_sin(sender, param, direction, ke, gvf_trajectories_config[sender]);
                break;
            }
            default:
                qDebug() << "GVF: GVF message parser received an unknown trajectory id.";
                return;
        }
    }

    // GVF_PARAMETRIC
    else if (msg.getDefinition().getName() == "GVF_PARAMETRIC") {
        QList<float> phi = {0.0}; // Error signals

        msg.getField("traj", traj);
        msg.getField("w", wb);
        msg.getField("p", param);
        msg.getField("phi", phi);

        switch(traj)
        {
            case 0: {// Trefoil 2D
                gvf_traj = new GVF_traj_trefoil(sender, param, phi, gvf_trajectories_config[sender]);
                break;
            }
            case 1: { // Ellipse 3D
                gvf_traj = new GVF_traj_3D_ellipse(sender, param, phi, gvf_trajectories_config[sender]);
                break;
            }
            case 2: { // Lissajous 3D
                gvf_traj = new GVF_traj_3D_lissajous(sender, param, phi, gvf_trajectories_config[sender]);
                break;
            }
            case 3: { // Bezier 2D
                gvf_traj = new GVF_traj_bezier(sender, param, phi, wb, gvf_trajectories_config[sender]);
                break;
            }
            default:
                qDebug() << "GVF: GVF_PARAMETRIC message parser received an unknown trajectory id.";
                return;
        }
    } else {
        return;
    }

    addItem(gvf_traj->getTraj());
    addItem(gvf_traj->getVField());
    ac_items_managers[sender]->setCurrentGVF(gvf_traj);
    gvf_trajectories[sender] = gvf_traj;
}

void MapWidget::onSLAM(QString sender, pprzlink::Message msg)
{

    if(!obstacles_visible) return; // Only paints if visible

    if(!AircraftManager::get()->aircraftExists(sender)) {
        return;
    }

    QList<float> obstacle_rel;
    uint8_t obstacle_type;
    msg.getField("obstacle", obstacle_rel);
    msg.getField("obstacle_type", obstacle_type);

    if(obstacle_rel.size() >= 2) {
        auto ac = AircraftManager::get()->getAircraft(sender);
        auto origin = ac->getFlightPlan()->getOrigin();

        if(!origin) return;
        Point2DLatLon ltp_origin(origin->getLat(), origin->getLon());
        Point2DLatLon wgs84_pos = CoordinatesTransform::get()->relative_utm_to_wgs84(ltp_origin, obstacle_rel[0], obstacle_rel[1]);

        // Properties of circle
        QColor line = QColor(Qt::black);
        QColor fill = QColor(Qt::black); // Known Obstacle
        if (obstacle_type == 1){
            fill = QColor(Qt::red);   // Unknown Obstacle
            line = QColor(Qt::red);
        }
        // qDebug() << "Type" << obstacle_type;

        fill.setAlpha(150);
        auto palette = PprzPalette(line, fill);
        float size = 0.1; // radius in meters

        double z = getAppSettings().value("map/z_values/shapes").toDouble();

        WaypointItem* center = new WaypointItem(wgs84_pos, "__NO_AC__", palette);
        center->setStyle(GraphicsObject::Style::CURRENT_NAV);
        addItem(center);

        CircleItem* circle = new CircleItem(center, size, "__NO_AC__", palette);
        circle->setOwnCenter(true);
        circle->setZValues(z, z);
        circle->setScalable(false);
        circle->setEditable(false);
        circle->setFilled(true);
        addItem(circle);

        center->setParent(circle); // to delete together

        const int MAX_OBSTACLES = 200;
        if(slam_obstacles.size() >= MAX_OBSTACLES) {
            auto old = slam_obstacles.takeFirst();
            removeItem(old.first);
            delete old.first;
        }

        // Save the new obstacle with its creation time
        slam_obstacles.append({circle, QDateTime::currentDateTime()});
    }
}

void MapWidget::onGridInit(QString sender, pprzlink::Message msg) {

    if(!AircraftManager::get()->aircraftExists(sender)) {
        return;
    }

    auto ac = AircraftManager::get()->getAircraft(sender);
    float cell_w, cell_h, xmin, xmax, ymin, ymax;
    int8_t lt;

    msg.getField("cell_width", cell_w);
    msg.getField("cell_height", cell_h);
    msg.getField("xmin", xmin);
    msg.getField("xmax", xmax);
    msg.getField("ymin", ymin);
    msg.getField("ymax", ymax);
    msg.getField("threshold", lt);

    int cols = qRound((xmax - xmin) / cell_w);
    int rows = qRound((ymax - ymin) / cell_h);

    if (!obstacle_grid_map) {
        obstacle_grid_map = new ObstacleGridMap(rows, cols);
    }
    if (!grid_item) {
        try {
            grid_item = new GridItem(ac->getId(), xmin, ymin, cell_w, cell_h, rows, cols, lt);
            grid_item->setGridMap(obstacle_grid_map);
            addItem(grid_item);
        } catch (const std::exception& e) {
            qCritical() << "Error creating GridItem:" << e.what();
        } catch (...) {
            qCritical() << "Unknown Error creating GridItem";
        }
    }

    grid_item->updateGraphics(this, UpdateEvent::ITEM_CHANGED);
}

void MapWidget::onGridChanges(QString sender, pprzlink::Message msg) {

    if(!AircraftManager::get()->aircraftExists(sender)) {
        return;
    }

    if (!grid_item || !obstacle_grid_map) {
        return;
    }

    uint8_t row, column;
    int8_t value;

    msg.getField("row", row);
    msg.getField("column", column);
    msg.getField("value", value);

    obstacle_grid_map->updateCell(row, column, value);
    grid_item->updateCell(this, UpdateEvent::ITEM_CHANGED, row, column);


}

void MapWidget::showHiddenWaypoints(bool state) {
    show_hidden_wp_action->blockSignals(true);
    show_hidden_wp_action->setChecked(state);
    show_hidden_wp_action->blockSignals(false);
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
}

void MapWidget::showCrashPrediction(bool state) {
    show_crash_prediction_action->blockSignals(true);
    show_crash_prediction_action->setChecked(state);
    show_crash_prediction_action->blockSignals(false);
    for(auto &itemManager: ac_items_managers) {
        if(state) {
            itemManager->getCrashItem()->setVisible(true);
        } else {
            itemManager->getCrashItem()->setVisible(false);
        }
    }
}
