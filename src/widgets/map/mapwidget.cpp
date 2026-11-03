#include "mapwidget.h"
#include "panels.h"
#include <cmath>
#include "map_notifications.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include "map_item.h"
#include <iostream>
#include <utility>
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


/// The resize grip in the bottom right corner of a rail card: drag to resize, double click to
/// go back to the size the panel asks for.
class CardGrip : public QWidget {
public:
    CardGrip(QWidget* card, std::function<void(QSize)> resized) : QWidget(card), card(card), resized(resized) {
        setFixedSize(16, 16);
        setCursor(Qt::SizeFDiagCursor);
        setToolTip("Drag to resize (double click: back to the default size)");
        card->installEventFilter(this);
        place();
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(palette().color(QPalette::Mid));
        for(int i=0; i<3; i++) {
            for(int j=0; j<=i; j++) {
                p.drawEllipse(QPointF(13 - j * 4, 13 - (i - j) * 4), 1.2, 1.2);
            }
        }
    }
    void mousePressEvent(QMouseEvent* e) override {
        start_pos = e->globalPosition().toPoint();
        start_size = card->size();
    }
    void mouseMoveEvent(QMouseEvent* e) override {
        if(!(e->buttons() & Qt::LeftButton)) {
            return;
        }
        QPoint d = e->globalPosition().toPoint() - start_pos;
        // not smaller than the panel needs (e.g. the PFD), not taller than the map
        QSize min_size = card->minimumSizeHint().expandedTo(QSize(240, 120));
        int max_h = card->parentWidget() ? card->parentWidget()->height() - 30 : 2000;
        QSize s(qMax(min_size.width(), start_size.width() + d.x()),
                qBound(min_size.height(), start_size.height() + d.y(), qMax(min_size.height(), max_h)));
        // a panel with a fixed shape (e.g. the PFD): its height follows the width, no white space
        if(card->layout() && card->layout()->hasHeightForWidth()) {
            s.setHeight(qBound(min_size.height(), card->layout()->totalHeightForWidth(s.width()), qMax(min_size.height(), max_h)));
            if(s.height() == max_h) {
                // as tall as allowed: as wide as that height needs
                for(int wd = s.width(); wd > min_size.width() && card->layout()->totalHeightForWidth(wd) > max_h; wd -= 4) {
                    s.setWidth(wd);
                }
            }
        }
        card->setMinimumSize(s);
        card->setMaximumSize(s);
    }
    void mouseReleaseEvent(QMouseEvent*) override {
        resized(card->size());
    }
    void mouseDoubleClickEvent(QMouseEvent*) override {
        resized(QSize());
    }
    bool eventFilter(QObject* o, QEvent* e) override {
        if(o == card && e->type() == QEvent::Resize) {
            place();
        }
        return false;
    }
private:
    void place() {
        move(card->width() - width() - 2, card->height() - height() - 2);
        raise();
    }
    QWidget* card;
    std::function<void(QSize)> resized;
    QPoint start_pos;
    QSize start_size;
};

/// Moving a rail card over the map by its title bar; a double click puts it back by the rail.
class CardMover : public QObject {
public:
    /// moved: while dragging (at most once per frame); finished: after it (to save it)
    CardMover(QWidget* header, QWidget* card, std::function<void(QPoint)> moved, std::function<void()> finished) :
        QObject(header), card(card), moved(moved), finished(finished) {
        frame_timer.setSingleShot(true);
        frame_timer.setInterval(16);
        connect(&frame_timer, &QTimer::timeout, this, [this]() {
            this->moved(target);
        });
        header->installEventFilter(this);
        header->setCursor(Qt::SizeAllCursor);
        header->setToolTip("Drag to move the panel over the map (double click: back by the rail)");
    }
protected:
    bool eventFilter(QObject*, QEvent* e) override {
        switch(e->type()) {
        case QEvent::MouseButtonPress: {
            auto me = static_cast<QMouseEvent*>(e);
            if(me->button() == Qt::LeftButton) {
                press = me->globalPosition().toPoint();
                start = card->pos();
                dragging = false;
                return true;    // not to the map below (it would pan)
            }
            return false;
        }
        case QEvent::MouseMove: {
            auto me = static_cast<QMouseEvent*>(e);
            if(!(me->buttons() & Qt::LeftButton)) {
                return false;
            }
            QPoint d = me->globalPosition().toPoint() - press;
            if(!dragging && d.manhattanLength() < 8) {
                return true;
            }
            dragging = true;
            target = start + d;
            if(!frame_timer.isActive()) {
                frame_timer.start();
            }
            return true;
        }
        case QEvent::MouseButtonRelease:
            if(dragging) {
                frame_timer.stop();
                moved(target);
                finished();
            }
            dragging = false;
            return true;
        case QEvent::MouseButtonDblClick:
            moved(QPoint(-1, -1));
            finished();
            return true;
        default:
            return false;
        }
    }
private:
    QWidget* card;
    std::function<void(QPoint)> moved;
    std::function<void()> finished;
    QTimer frame_timer;
    QPoint target;
    QPoint press;
    QPoint start;
    bool dragging = false;
};

MapWidget::MapWidget(QWidget *parent) : Map2D(parent),
    interaction_state(PMIS_OTHER), drawState(false), fp_edit_sm(nullptr), gcsItem(nullptr),
    pan_state(PAN_IDLE), pan_mouse_mask(Qt::MiddleButton | Qt::LeftButton),
    _ac_arrow_size(30)
{

    auto settings = getAppSettings();

    // main layout: the rail, panels and buttons over the map, and panels along its bottom
    auto main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);
    horizontalLayout = new QHBoxLayout();
    main_layout->addLayout(horizontalLayout, 1);
    bottom_tabs = new PanelTabs("map_bottom", this, false);
    bottom_tabs->setMaximumHeight(340);
    bottom_tabs->setAutoFillBackground(true);
    main_layout->addWidget(bottom_tabs);
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

    // the buttons on the left are a full-height rail along the edge of the map
    auto rail = new QWidget(this);
    rail->setObjectName("map_rail");
    rail->setAttribute(Qt::WA_StyledBackground);
    rail->setLayout(buttonsLeftLayout);
    buttonsLeftLayout->setContentsMargins(4, 6, 4, 6);
    buttonsLeftLayout->setSpacing(4);
    if(!qApp->styleSheet().contains("map_rail")) {
        rail->setStyleSheet("#map_rail {background: #22272e;}");
    }
    horizontalLayout->setContentsMargins(0, 0, 9, 0);
    columnLeft->setContentsMargins(6, 9, 0, 9);
    rightLayout->setContentsMargins(0, 9, 0, 9);
    horizontalLayout->addWidget(rail);
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
    connect(timer_intruders, &QTimer::timeout, this, [=,this]() {
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
        [=,this](SmEditEvent eventType, QGraphicsSceneMouseEvent *mouseEvent) {
            if(interaction_state == PMIS_FLIGHT_PLAN_EDIT && fp_edit_sm != nullptr) {
                MapItem* item = fp_edit_sm->update(eventType, mouseEvent, nullptr, current_ac);
                (void)item; //put item in a list relative to the drone (in a drone FP, in a block)
            }
        });


    // Context menu
    mapMenu = new QMenu("Map", pprzApp()->mainWindow());

    auto copy_coordinates = mapMenu->addAction("Copy coordinates");
    connect(copy_coordinates, &QAction::triggered, this, [=,this](){
        auto mc = getMouseCoords();
        auto clipboard = QApplication::clipboard();
        auto mc_txt = QString("%1, %2").arg(mc.lat(), 0, 'f', 7).arg(mc.lon(), 0, 'f', 7);
        clipboard->setText(mc_txt);
    });

    show_hidden_wp_action = mapMenu->addAction("Show hidden waypoints");
    show_hidden_wp_action->setCheckable(true);
    connect(show_hidden_wp_action, &QAction::toggled, [=,this](bool show) {
        setProperty("show_hidden_waypoints", show);
    });

    show_crash_prediction_action = mapMenu->addAction("Show crash prediction");
    show_crash_prediction_action->setCheckable(true);
    connect(show_crash_prediction_action, &QAction::toggled, [=,this](bool show) {
        setProperty("show_crash_prediction", show);
    });

    auto clear_shapes = mapMenu->addAction("Clear Shapes");
    connect(clear_shapes, &QAction::triggered, this, [=,this](){
        clearShapes();
    });

    auto clear_dcshots = mapMenu->addAction("Clear DCSHOTS");
    connect(clear_dcshots, &QAction::triggered, this, [=,this](){
        clearDcShots();
    });

    menu_clear_track = new QMenu("Clear Track", mapMenu);
    mapMenu->addMenu(menu_clear_track);

    // Shortcuts: only when the map has the focus (click on it), so keys pressed in the strips,
    // the checklist or a text field never change the map. They are listed in Map > Shortcuts.
    auto shortcuts_menu = mapMenu->addMenu("Shortcuts");
    auto add_shortcut = [=,this](QString text, QKeySequence key, std::function<void()> f) {
        auto action = new QAction(text, this);
        action->setShortcut(key);
        action->setShortcutContext(Qt::WidgetShortcut);
        connect(action, &QAction::triggered, this, f);
        this->addAction(action);
        shortcuts_menu->addAction(action);
        return action;
    };
    add_shortcut("Rotate map +10\u00b0", QKeySequence(Qt::Key_R), [=,this](){ rotateMap(10); });
    add_shortcut("Rotate map -10\u00b0", QKeySequence(Qt::Key_R | Qt::CTRL), [=,this](){ rotateMap(-10); });
    add_shortcut("Rotate map +5\u00b0", QKeySequence(Qt::Key_R | Qt::SHIFT), [=,this](){ rotateMap(5); });
    add_shortcut("Rotate map -5\u00b0", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R), [=,this](){ rotateMap(-5); });
    add_shortcut("North up", QKeySequence(Qt::Key_R | Qt::ALT), [=,this](){ rotateMap(-getRotation()); });
    add_shortcut("Center on the aircraft", QKeySequence(Qt::Key_C), [=,this](){
        if(AircraftManager::get()->aircraftExists(current_ac)) {
            auto ac = AircraftManager::get()->getAircraft(current_ac);
            auto orig = ac->getFlightPlan()->getOrigin();
            Point2DLatLon pos(orig);
            if(ac->isReal()) {
                pos = ac->getPosition();
            }
            centerLatLon(pos);
        }
    });
    add_shortcut("Freeze waypoints (toggle)", QKeySequence(Qt::Key_F), [=,this](){
        setFrozen(interaction_state != PMIS_FROZEN);
    });
    add_shortcut("Unfreeze / cancel", QKeySequence(Qt::Key_Escape), [=,this](){
        if(interaction_state == PMIS_FLIGHT_PLAN_EDIT && fp_edit_sm != nullptr) {
            MapItem* item = fp_edit_sm->update(FPEE_CANCEL, nullptr, nullptr, current_ac);
            (void)item;
        }
        setMouseTracking(false);
        scene()->setShortcutItems(false);
        setFrozen(false);
        drawState = 0;
        setCursor(Qt::ArrowCursor);
    });
    shortcuts_menu->addSeparator();
    auto zoom_info = shortcuts_menu->addAction("Zoom: + / - or the mouse wheel");
    zoom_info->setEnabled(false);
    auto add_info = shortcuts_menu->addAction("Add a waypoint (flight plan editor): Ctrl + click");
    add_info->setEnabled(false);

    // banner while the waypoints are frozen
    frozen_banner = new QPushButton("Waypoints frozen \u00b7 F or Esc to unfreeze", this);
    frozen_banner->setObjectName("map_mode_banner");
    frozen_banner->setCursor(Qt::PointingHandCursor);
    frozen_banner->setToolTip("Click to unfreeze");
    frozen_banner->hide();
    connect(frozen_banner, &QPushButton::clicked, this, [=,this]() { setFrozen(false); });

    // Connect signals
    connect(AircraftManager::get(), &AircraftManager::waypoint_changed, this, &MapWidget::onWaypointChanged);
    connect(AircraftManager::get(), &AircraftManager::waypoint_added, this, &MapWidget::onWaypointAdded);
    connect(DispatcherUi::get(), &DispatcherUi::move_waypoint_ui, this, &MapWidget::onMoveWaypointUi);
    connect(DispatcherUi::get(), &DispatcherUi::slamGridVisibilityChanged, this,
        [=,this](bool visible) {
            if(grid_item) {
                grid_item->setVisible(visible);
                
            }
        });
    connect(DispatcherUi::get(), &DispatcherUi::obstacleVisibilityChanged, this,
        [=,this](bool visible) {
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
        [=,this](QString sender, QVector<int>* gvfViewer_config) {
            gvf_trajectories_config.remove(sender);
            gvf_trajectories_config[sender] = gvfViewer_config;
        });

    connect(  DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &MapWidget::handleNewAC);
    connect(  DispatcherUi::get(), &DispatcherUi::ac_deleted, this, &MapWidget::removeAC);
    connect(  DispatcherUi::get(), &DispatcherUi::ac_selected, this, &MapWidget::changeCurrentAC);
    connect(  DispatcherUi::get(), &DispatcherUi::centerMap, this, &MapWidget::centerLatLon);
    connect(PprzDispatcher::get(), &PprzDispatcher::flight_param, this, &MapWidget::updateAircraftItem);
    // rotorcraft: the position at the telemetry rate, in the local frame of INS_REF
    PprzDispatcher::get()->bindOptional("ROTORCRAFT_FP", this, [=,this](QString sender, pprzlink::Message msg) {
        if(!AircraftManager::get()->aircraftExists(sender) || !ac_items_managers.contains(sender)) {
            return;
        }
        auto ref = AircraftManager::get()->getAircraft(sender)->getStatus()->getInsRef();
        if(!ref) {
            return;     // not the origin yet: FLIGHT_PARAM
        }
        int32_t east, north, psi;
        msg.getField("east", east);
        msg.getField("north", north);
        msg.getField("psi", psi);
        const double pos_coef = 0.0039063;      // POS_BFP to m
        const double angle_coef = 0.0139882;    // ANGLE_BFP to deg
        auto pos = CoordinatesTransform::get()->ltp_to_wgs84(Point2DLatLon(ref->lat, ref->lon),
                                                             east * pos_coef, north * pos_coef);
        direct_position[sender].start();
        setAircraftPose(sender, pos, psi * angle_coef);
    });
    connect(PprzDispatcher::get(), &PprzDispatcher::nav_status, this, &MapWidget::updateTarget);
    connect(PprzDispatcher::get(), &PprzDispatcher::circle_status, this, &MapWidget::updateNavShape);
    connect(PprzDispatcher::get(), &PprzDispatcher::segment_status, this, &MapWidget::updateNavShape);


    shape_bind_id = PprzDispatcher::get()->bind("SHAPE", this,
        [=,this](QString sender, pprzlink::Message msg) {
            onShape(sender, msg);
        });

    PprzDispatcher::get()->bind("INTRUDER", this,
        [=,this](QString sender, pprzlink::Message msg) {
            onIntruder(sender, msg);
        });

    PprzDispatcher::get()->bind("DC_SHOT", this,
        [=,this](QString sender, pprzlink::Message msg) {
            onDcShot(sender, msg);
        });

    PprzDispatcher::get()->bind("FLIGHT_PARAM", this,
        [=,this](QString sender, pprzlink::Message msg) {
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
        [=,this](QString sender, pprzlink::Message msg) {
            onGVF(sender, msg);
        });

    PprzDispatcher::get()->bind("GVF_PARAMETRIC", this,
        [=,this](QString sender, pprzlink::Message msg) {
            onGVF(sender, msg);
        });

    PprzDispatcher::get()->bind("ROTORCRAFT_FP", this,
        [=,this](QString sender, pprzlink::Message msg) {
            onROTORCRAFT_FP(sender, msg);
        });

    PprzDispatcher::get()->bind("SLAM", this,
        [=,this](QString sender, pprzlink::Message msg) {
            onSLAM(sender, msg);
        });

    PprzDispatcher::get()->bind("GRID_INIT", this,
        [=,this](QString sender, pprzlink::Message msg) {
            onGridInit(sender, msg);
        });

    PprzDispatcher::get()->bind("GRID_CHANGES", this,
        [=,this](QString sender, pprzlink::Message msg) {
            onGridChanges(sender, msg);
        });

    setAcceptDrops(true);

    // Add menu to app menu bar.
    connect(pprzApp()->mainWindow(), &PprzMain::ready, this, [=,this](){
        auto actions = pprzApp()->mainWindow()->menuBar()->actions();
        int index = actions.size()-1;
        if(index < 0) {
            pprzApp()->mainWindow()->menuBar()->addMenu(mapMenu);
        } else {
            pprzApp()->mainWindow()->menuBar()->insertMenu(actions[index], mapMenu);
        }
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
        [=,this](QPointF pos) {
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
        layer_combo->makeLayerControl(tp->config()->name, visible, z, tp->config()->title);
    }

    // the map brightness: the imagery a bit darker makes what is drawn on it stand out
    auto dim_row = new QWidget(layer_combo);
    dim_row->setObjectName("map_brightness");
    auto dim_lay = new QHBoxLayout(dim_row);
    dim_lay->setContentsMargins(8, 6, 8, 6);
    auto dim_label = new QLabel("Map brightness", dim_row);
    auto dim_slider = new QSlider(Qt::Horizontal, dim_row);
    dim_slider->setRange(30, 100);
    int dim = settings.value("map/dim", 20).toInt();
    dim_slider->setValue(100 - dim);
    dim_slider->setToolTip("Darker imagery makes the aircraft and waypoints stand out");
    dim_lay->addWidget(dim_label);
    dim_lay->addWidget(dim_slider, 1);
    layer_combo->addTopWidget(dim_row);
    connect(dim_slider, &QSlider::valueChanged, this, [=,this](int brightness) {
        setMapDim(100 - brightness);
        auto s = getAppSettings();
        s.setValue("map/dim", 100 - brightness);
    });
    setMapDim(dim);

    connect(
        layer_combo, &LayerCombo::showLayer, this,
        [=,this](QString name, bool state) {
            toggleTileProvider(name, state);
            updateTiles();
        }
    );

    connect(
        layer_combo, &LayerCombo::layerOpacityChanged, this,
        [=,this](QString name, qreal opacity) {
            setLayerOpacity(name, opacity);
        }
    );

    connect(
        layer_combo, &LayerCombo::zValueChanged, this,
        [=,this](QString name, int z) {
            setLayerZ(name, z);
        }
    );

    return layer_combo;

    //auto button = new LockButton(QIcon(settings.value("APP_DATA_PATH").toString() + "/pictures/" + "map_layers_normal.svg"), this);

    //addWidget(layer_combo, button, WIDGETS_LEFT);
}

void MapWidget::addWidget(QWidget* widget, LockButton* button, WidgetContainer side, int index) {
    QList<LockButton*>* buttons;
    if(side == WIDGETS_LEFT) {
        columnLeft->addWidget(widget);
        if(index < 0 || index > buttonsLeft.size()) {
            index = buttonsLeft.size();
        }
        buttonsLeftLayout->insertWidget(index, button);
        buttonsLeft.insert(index, button);
        buttons = &buttonsLeft;
    } else  {
        columnRight->addWidget(widget);
        buttonsRightLayout->addWidget(button);
        buttons = &buttonsRight;
        columnRight->setAlignment(widget, Qt::AlignRight);
    }

    widget->hide();
    button->setWidget(widget);
    if(side != WIDGETS_LEFT) {
        buttons->append(button);
    }

    // Widgets that can have unseen content (e.g. chat) show a notification bubble on their button.
    if(widget->metaObject()->indexOfSignal("unreadChanged(int)") != -1) {
        connect(widget, SIGNAL(unreadChanged(int)), button, SLOT(setBadge(int)));
    }

    connect(
        button, &LockButton::clicked,
        [=,this](bool active) {
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

            // name on hover: the "title" attribute, else the widget name ("gps_classic_viewer" -> "Gps classic viewer")
            auto title = widget_ele.attribute("title");
            if(title.isEmpty()) {
                title = name;
                title.replace('_', ' ');
                title[0] = title[0].toUpper();
            }

            if(side == WIDGETS_LEFT) {
                // the rail: panels that can be dragged to the tabs or the bottom of the map
                widget->setObjectName(id);
                PanelManager::get()->registerPanel(id, widget, title, icon_src, nullptr);
                addRailPanel(id, widget, title, icon_src);
            } else {
                auto icon = QIcon(user_or_app_path("pictures/" + icon_src));
                auto button = new LockButton(icon, this);
                button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                button->setToolTip(title + " (long press or the lock in the corner: keep it open)");
                addWidget(widget, button, side);
            }
            installEventFilter(widget);

        }
    }

    PanelManager::get()->setMap(this);

    // notifications at the top of the map: messages, low battery, lost links
    notifications = new MapNotifications(this);
    for(auto card: rail_cards) {
        notifications->avoid(card);
    }
    connect(notifications, &MapNotifications::openChat, this, []() {
        PanelManager::get()->showPanel("chat");
    });
    connect(notifications, &MapNotifications::senderClicked, this, [=,this](QString sender) {
        PanelManager::get()->showPanel("chat");     // the message in the chat
        if(AircraftManager::get()->aircraftExists(sender)) {
            emit DispatcherUi::get()->ac_selected(sender);
            auto ac = AircraftManager::get()->getAircraft(sender);
            if(ac->isReal()) {
                centerLatLon(ac->getPosition());
            }
        }
    });
    PprzDispatcher::get()->bind("INFO_MSG", this, [=,this](QString sender, pprzlink::Message msg) {
        QByteArray ar;
        msg.getField("msg", ar);
        notifications->message(sender, QString(ar));
    });
    {
        QSettings app_settings(appConfig()->value("SETTINGS_PATH").toString(), QSettings::IniFormat);
        QString own_id = app_settings.value("pprzlink/id").toString();
        PprzDispatcher::get()->bind("INFO_MSG_GROUND", this, [=,this](QString sender, pprzlink::Message msg) {
            (void)sender;
            QString txt, source, dest;
            msg.getField("msg", txt);
            msg.getField("source", source);
            msg.getField("dest", dest);
            auto d = dest.split(':')[0];
            bool ok;
            int dest_id = d.toInt(&ok, 0);
            // to us, the ground or all; not our own messages
            if(source != own_id && (d == own_id || (ok && (dest_id == 0 || dest_id == 0xFF)))) {
                notifications->message(source, txt);
            }
        });
    }
    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &MapWidget::watchAircraftStates);
    // an aircraft that is removed: its states go
    connect(DispatcherUi::get(), &DispatcherUi::ac_deleted, this, [=,this](QString id) {
        notifications->setState("bat:" + id, id, "", MapNotifications::INFO);
        notifications->setState("link:" + id, id, "", MapNotifications::INFO);
    });
    for(auto ac: AircraftManager::get()->getAircrafts()) {
        watchAircraftStates(ac->getId());
    }

    wind_indicator = new WindIndicator(this);
    wind_indicator->setObjectName("wind_indicator");
    wind_indicator->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    rightLayout->addStretch(1);  // at the bottom, also when there are no panels on the right
    rightLayout->addWidget(wind_indicator, 0, Qt::AlignRight);

    PprzDispatcher::get()->bind("WIND", this, [=,this](QString sender, pprzlink::Message msg){
        (void)sender;
        QString ac_id;
        msg.getField("ac_id", ac_id);
        double dir = getFloatingField(msg, "dir");
        double wspeed = getFloatingField(msg, "wspeed");
        // double mean_aspeed = getFloatingField(msg, "mean_aspeed");
        // double stddev = getFloatingField(msg, "stddev");
        wind_indicator->setWindData(ac_id, dir, wspeed);
    });

    connect(wind_indicator, &WindIndicator::requestRotation, this, [=,this](double rot) {
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

    connect(map_item, &MapItem::itemChanged, map_item, [=,this]() {
        map_item->updateGraphics(this, UpdateEvent::ITEM_CHANGED);
        if(qobject_cast<WaypointItem*>(map_item) && wp_labels_timer) {
            wp_labels_timer->start();
        }
    });

    connect(map_item, &MapItem::itemGainedHighlight, map_item, [=,this]() {
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
    for(auto item: std::as_const(_items)) {
        item->setForbidHighlight(fh);
    }
}

void MapWidget::itemsEditable(bool ed) {
    for(auto item: std::as_const(_items)) {
        item->setEditable(ed);
    }
}

void MapWidget::updateHighlights(QString ac_id) {
    for(auto item: std::as_const(_items)) {
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
        QList<QEventPoint> touchPoints = touchEvent->points();

        for(auto &touchPoint: touchPoints) {
            if(touchPoint.state() == QEventPoint::State::Pressed) {
                auto scenePos = mapToScene(touchPoint.position().toPoint());
                auto tp = tilePoint(scenePos, zoomLevel(zoom()), tileSize());
                pms[touchPoint.id()] = Point2DPseudoMercator(tp);
            }
            else if(touchPoint.state() == QEventPoint::State::Released && pms.contains(touchPoint.id())) {
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

            auto px_dist = QVector2D(tp1->position() - tp0->position()).length();

            for(int zo=0; zo<25; zo++) {
                auto pt1 = scenePoint(pms[id0], zo, tileSize());
                auto pt2 = scenePoint(pms[id1], zo, tileSize());
                auto dist = QVector2D(pt2-pt1).length();
                if(dist > px_dist) {
                    // found the right zoom level
                    double s = px_dist/dist;
                    double new_zoom = zo + log2(s);

                    auto center = (tp1->position() + tp0->position())/2;
                    auto pmc = (pms[id0] + pms[id1])/2;
                    zoomCenteredScene(new_zoom, center.toPoint(), pmc);
                    updateGraphics(UpdateEvent::MAP_ZOOMED|UpdateEvent::MAP_MOVED|UpdateEvent::MAP_ROTATED);
                    break;
                }
            }
            return true;
        } else if(touchPoints.count() == 1 && touchPoints.first().state() == QEventPoint::State::Updated) {
            //Pan only
            auto pm = pms[touchPoints.first().id()];
            auto pos = touchPoints.first().position().toPoint();
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
    for(auto item: std::as_const(_items)) {
        item->updateGraphics(this, update_events);
    }
    for(auto papget: std::as_const(papgets)) {
        papget->updateGraphics(this);
    }
    for(auto wpi: pending_moves.keys()) {
        updatePendingMove(wpi);
    }
    arrangeAircraftLabels();
    if(wp_labels_timer == nullptr) {
        // after a burst of changes (moves, zoom), once
        wp_labels_timer = new QTimer(this);
        wp_labels_timer->setSingleShot(true);
        wp_labels_timer->setInterval(30);
        connect(wp_labels_timer, &QTimer::timeout, this, &MapWidget::arrangeWaypointLabels);
    }
    wp_labels_timer->start();
}

void MapWidget::arrangeWaypointLabels() {
    struct Wp {WaypointItem* item; QPointF center; QSizeF size;};
    QList<Wp> wps;
    QList<QRectF> markers;
    for(auto mgr: ac_items_managers) {
        for(auto wpi: mgr->getWaypointsItems()) {
            auto marker = wpi->marker();
            if(marker == nullptr || !marker->isVisible()) {
                continue;
            }
            // the visible diamond (the bounding box has room for the highlight and shadow)
            QRectF mr = marker->sceneBoundingRect();
            markers.append(mr.adjusted(mr.width() * 0.22, mr.height() * 0.22, -mr.width() * 0.22, -mr.height() * 0.22));
            auto label = wpi->label();
            if(label && label->isVisible() && !label->toPlainText().isEmpty()) {
                QRectF r = label->boundingRect();
                // its exact position (as WaypointItem places the name), not the middle of its box
                wps.append({wpi, wpi->scenePos(), QSizeF(r.width(), r.height()) * label->scale()});
            }
        }
    }
    // the aircraft: a name under one is unreadable
    QList<QRectF> aircraft;
    for(auto mgr: ac_items_managers) {
        auto ai = mgr->getAircraftItem();
        if(ai != nullptr && ai->icon() != nullptr && ai->icon()->isVisible()) {
            QRectF r = ai->icon()->sceneBoundingRect();
            aircraft.append(r.adjusted(r.width() * 0.15, r.height() * 0.15, -r.width() * 0.15, -r.height() * 0.15));
        }
    }
    // top to bottom, left to right: stable from one update to the next
    std::sort(wps.begin(), wps.end(), [](const Wp& a, const Wp& b) {
        return a.center.y() < b.center.y() || (a.center.y() == b.center.y() && a.center.x() < b.center.x());
    });
    auto rot = QTransform().rotate(-getRotation());
    QList<QRectF> placed;
    for(auto &wp: wps) {
        double s = wp.item->label()->scale();     // the waypoint scale (set with its label)
        double w = wp.size.width(), h = wp.size.height();
        // upper right (the default), lower right, upper left, lower left, above, below
        QList<QPointF> offsets = {
            {4*s, -26*s}, {4*s, 0}, {-w - 4*s, -26*s}, {-w - 4*s, 0}, {-w/2, -h - 6*s}, {-w/2, 8*s},
        };
        // the place it has now first: it stays there while that is free (no jumping around)
        offsets.prepend(wp.item->labelOffset() * s);
        QPointF chosen = offsets[0];
        QRectF chosen_rect;
        for(auto &o: offsets) {
            QRectF r(wp.center + rot.map(o), wp.size);
            QRectF rr = r.adjusted(w * 0.08, h * 0.15, -w * 0.08, -h * 0.15);   // the text, not its margins
            bool free = true;
            for(auto &p: placed) {
                free = free && !p.intersects(rr);
            }
            for(auto &m: markers) {
                if(m.contains(wp.center)) {
                    continue;   // its own waypoint (or one at the same place)
                }
                free = free && !m.intersects(rr);
            }
            for(auto &a: aircraft) {
                free = free && !a.intersects(rr);
            }
            if(free) {
                chosen = o;
                chosen_rect = rr;
                break;
            }
        }
        if(chosen_rect.isNull()) {
            QRectF r(wp.center + rot.map(chosen), wp.size);
            chosen_rect = r.adjusted(w * 0.08, h * 0.15, -w * 0.08, -h * 0.15);
        }
        wp.item->setLabelOffset(chosen / s);
        wp.item->label()->setPos(wp.center + rot.map(chosen));
        placed.append(chosen_rect);
    }
}

void MapWidget::arrangeAircraftLabels() {
    // the names in a pill of their color when there are several aircraft
    int nb_real = 0;
    for(auto ac: AircraftManager::get()->getAircrafts()) {
        nb_real += ac->isReal() ? 1 : 0;
    }
    // the names only while hovering an aircraft, or when it overlaps another one (which is which)
    QList<AircraftItem*> items;
    for(auto ac_id: ac_items_managers.keys()) {
        auto ai = ac_items_managers[ac_id]->getAircraftItem();
        if(ai != nullptr && ai->label() != nullptr && ai->icon() != nullptr) {
            items.append(ai);
        }
    }
    for(auto ai: items) {
        ai->label()->setPill(nb_real > 1);
        bool show = ai->isHovered();
        QRectF r = ai->icon()->sceneBoundingRect();
        for(auto other: items) {
            if(other != ai && other->icon()->isVisible() && other->icon()->sceneBoundingRect().intersects(r)) {
                show = true;
            }
        }
        ai->label()->setOpacity(show ? 1 : 0);
    }
    QList<QRectF> placed;
    for(auto ac_id: ac_items_managers.keys()) {
        auto ai = ac_items_managers[ac_id]->getAircraftItem();
        if(ai == nullptr || ai->label() == nullptr || !ai->label()->isVisible() || ai->label()->opacity() == 0) {
            continue;
        }
        auto label = ai->label();
        // from its default place (not from where it was moved last time: no jumping around)
        double sc = label->scale() > 0 ? label->scale() : 1;
        double shift = 0;
        QRectF r = label->sceneBoundingRect().translated(0, -ai->labelShift() * sc);
        // below the labels it overlaps
        for(int tries = 0; tries < 8; tries++) {
            bool moved = false;
            for(auto &p: placed) {
                if(p.intersects(r)) {
                    double dy = p.bottom() - r.top() + 1;
                    r.translate(0, dy);
                    shift += dy;
                    moved = true;
                }
            }
            if(!moved) {
                break;
            }
        }
        if(fabs(shift / sc - ai->labelShift()) > 0.01) {
            label->setPos(label->pos() + QPointF(0, shift - ai->labelShift() * sc));
            ai->setLabelShift(shift / sc);
        }
        placed.append(r);
    }
}

void MapWidget::setMapDim(int percent) {
    if(dim_rect == nullptr) {
        dim_rect = scene()->addRect(QRectF(), Qt::NoPen, Qt::black);
        dim_rect->setZValue(45);    // above the imagery, below the shapes and waypoints
        dim_rect->setAcceptedMouseButtons(Qt::NoButton);
        dim_rect->setRect(scene()->sceneRect());
        connect(scene(), &QGraphicsScene::sceneRectChanged, this, [=,this](const QRectF& r) {
            dim_rect->setRect(r);
        });
    }
    dim_rect->setOpacity(qBound(0, percent, 70) / 100.);
    dim_rect->setVisible(percent > 0);
}

void MapWidget::showPendingMove(WaypointItem* wpi) {
    if(pending_moves.contains(wpi)) {
        return;     // still waiting for the confirmation of an earlier move: keep its old position
    }
    QPen pen(Qt::white, 2, Qt::DashLine);
    pen.setCosmetic(true);
    PendingMove pm;
    pm.from = Point2DLatLon(wpi->getOriginalWaypoint());
    pm.marker = scene()->addPolygon(QPolygonF({QPointF(0, -8), QPointF(8, 0), QPointF(0, 8), QPointF(-8, 0)}), pen);
    pm.marker->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    pm.marker->setZValue(900);
    pm.marker->setToolTip("Position before the move, until the aircraft confirms it");
    pm.line = scene()->addLine(QLineF(), pen);
    pm.line->setZValue(899);
    pm.conn = connect(wpi, &WaypointItem::itemChanged, this, [=,this]() { updatePendingMove(wpi); });
    pending_moves[wpi] = pm;
    updatePendingMove(wpi);
}

void MapWidget::updatePendingMove(WaypointItem* wpi) {
    if(!pending_moves.contains(wpi)) {
        return;
    }
    auto &pm = pending_moves[wpi];
    QPointF old_pos = scenePoint(pm.from, zoomLevel(zoom()), tileSize());
    pm.marker->setPos(old_pos);
    pm.line->setLine(QLineF(old_pos, wpi->scenePos()));
}

void MapWidget::clearPendingMove(WaypointItem* wpi) {
    if(!pending_moves.contains(wpi)) {
        return;
    }
    auto pm = pending_moves.take(wpi);
    disconnect(pm.conn);
    scene()->removeItem(pm.marker);
    scene()->removeItem(pm.line);
    delete pm.marker;
    delete pm.line;
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

        Papget* papget = new Papget(datadef, event->position().toPoint());
        scene()->addItem(papget);
        papget->setZValue(1000);
        papgets.append(papget);

        connect(papget, &Papget::moved, this, [=,this](QPointF pos) {
            QPoint viewPos = mapFromScene(pos);
            papget->setPosition(viewPos);
        });

        connect(papget, &Papget::removeRequested, this, [=,this]() {
            papgets.removeAll(papget);
            scene()->removeItem(papget);
            papget->deleteLater();
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
        connect(aircraft_item, &AircraftItem::hoverChanged, this, &MapWidget::arrangeAircraftLabels);

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

        connect(arrow, &ArrowItem::centerAC, this, [=,this]() {
            auto pos = ac->getPosition();
            centerLatLon(pos);
        });

        //create the ACItemManager for this aircraft
        item_manager = new ACItemManager(ac_id, target, aircraft_item, arrow, crash_item, this);

        auto clear_track = new QAction(ac->name(), ac);
        connect(clear_track, &QAction::triggered, aircraft_item, [=,this](){
            aircraft_item->clearTrack();
        });
        menu_clear_track->addAction(clear_track);

        // a new INS reference (e.g. at boot): no track line from the old position
        connect(ac->getStatus(), &AircraftStatus::ins_ref, aircraft_item, [=]() {
            aircraft_item->clearTrack();
        });

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
    for(auto wpi: pending_moves.keys()) {
        if(wpi->acId() == ac_id) {
            clearPendingMove(wpi);
        }
    }
    ac_items_managers[ac_id]->removeItems(this);
    ac_items_managers[ac_id]->deleteLater();
    ac_items_managers.remove(ac_id);

    if(gvf_loaded) {
        gvf_trajectories.remove(ac_id);
        gvf_trajectories_config.remove(ac_id);
    }
}

void MapWidget::onWaypointChanged(Waypoint* wp, QString ac_id) {
    if(AircraftManager::get()->aircraftExists(ac_id)) {
        for(auto wpi: ac_items_managers[ac_id]->getWaypointsItems()) {
            if(wpi->getOriginalWaypoint() == wp && !wpi->isMoving()) {
                if(pending_moves.contains(wpi)) {
                    // the server sends all waypoints periodically: only the position asked
                    // confirms the move, an older one keeps showing the move as pending
                    double d = 0, az = 0;
                    CoordinatesTransform::get()->distance_azimut(Point2DLatLon(wp), Point2DLatLon(wpi->waypoint()), d, az);
                    if(d > 1.0 || fabs(wp->getAlt() - wpi->waypoint()->getAlt()) > 0.5) {
                        continue;
                    }
                }
                wpi->update();
                wpi->setAnimate(false);
                clearPendingMove(wpi);     // confirmed
            }
        }
    }
}

void MapWidget::onWaypointAdded(Waypoint* wp, QString ac_id) {
    WaypointItem* wpi = new WaypointItem(wp, ac_id);
    addItem(wpi);
    ac_items_managers[ac_id]->addWaypointItem(wpi);

    auto dialog_move_waypoint = [=,this]() {
        wpi->setMoving(true);
        auto we = new WaypointEditor(wpi, ac_id, this);
        // the old position: kept until the aircraft confirms the move (also over several moves)
        showPendingMove(wpi);

        // the card next to the waypoint, not over it, its old position or the line between them
        auto view_pos = mapFromScene(wpi->scenePos());
        QRect avoid;
        if(pending_moves.contains(wpi)) {
            auto from = mapToGlobal(mapFromScene(pending_moves[wpi].marker->pos()));
            avoid = QRect(from, mapToGlobal(view_pos)).normalized();
        }
        we->placeNear(mapToGlobal(view_pos), avoid);

        connect(we, &QDialog::finished, wpi, [=,this](int result) {
            wpi->setMoving(false);
            if(!result) {
                // back to the confirmed position
                wpi->update();
                wpi->setAnimate(false);
                clearPendingMove(wpi);
            }
        });
        we->showMovable();
    };

    connect(wpi, &WaypointItem::waypointMoveFinished, this, dialog_move_waypoint);

    connect(wpi, &WaypointItem::itemDoubleClicked, this, dialog_move_waypoint);

    connect(wpi, &WaypointItem::waypointMoved, this, [=,this](){
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
                clearPendingMove(wpi);
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

    auto delete_prev = [=,this]() {
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

    // a broken position (NaN, out of range) would move the aircraft out of the map
    if(!std::isfinite(lat) || !std::isfinite(lon) || !std::isfinite(heading) || fabs(lat) > 90 || fabs(lon) > 180) {
        return;
    }

    // the position from the telemetry is newer (when it came in the last 2 s)
    if(direct_position.contains(ac_id) && direct_position[ac_id].isValid() && direct_position[ac_id].elapsed() < 2000) {
        return;
    }
    setAircraftPose(ac_id, Point2DLatLon(lat, lon), heading);
}

void MapWidget::setAircraftPose(QString ac_id, Point2DLatLon pos, double heading) {
    if(!std::isfinite(pos.lat()) || !std::isfinite(pos.lon()) || !std::isfinite(heading) ||
       fabs(pos.lat()) > 90 || fabs(pos.lon()) > 180) {
        return;
    }
    if(AircraftManager::get()->aircraftExists(ac_id) && ac_items_managers.contains(ac_id)) {
        auto ai = ac_items_managers[ac_id]->getAircraftItem();
        auto arrow = ac_items_managers[ac_id]->getArrowItem();
        if(ai == nullptr || arrow == nullptr) {
            return;
        }
        ai->setPosition(pos);
        arrow->setAcPos(pos);
        ai->setHeading(heading);
        arrangeAircraftLabels();
        if(wp_labels_timer) {
            wp_labels_timer->start();     // the waypoint names away from the aircraft
        }
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

void MapWidget::addRailPanel(QString id, QWidget* widget, QString title, QString icon, int index) {
    auto button = new LockButton(QIcon(user_or_app_path("pictures/" + icon)), this);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    button->setToolTip(title + " (long press: keep it open; drag: move it to the tabs or the bottom of the map)");

    // the panel in a card over the map: a title bar with a close button
    auto card = new QFrame(this);
    card->setObjectName("rail_card");
    card->setAttribute(Qt::WA_StyledBackground);
    auto card_layout = new QVBoxLayout(card);
    card_layout->setContentsMargins(0, 0, 0, 0);
    card_layout->setSpacing(0);
    auto header = new QWidget(card);
    header->setObjectName("rail_card_header");
    auto header_layout = new QHBoxLayout(header);
    header_layout->setContentsMargins(10, 2, 2, 2);
    auto title_label = new QLabel(title, header);
    title_label->setObjectName("rail_card_title");
    auto close_button = new QToolButton(header);
    close_button->setObjectName("rail_card_close");
    close_button->setText(QString::fromUtf8("\xE2\x9C\x95"));
    close_button->setToolTip("Close");
    close_button->setCursor(Qt::ArrowCursor);   // not the move cursor of the title bar
    header_layout->addWidget(title_label, 1);
    header_layout->addWidget(close_button);
    header->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    card_layout->addWidget(header);
    auto body = new QWidget(card);
    body->setObjectName("rail_card_body");
    auto body_layout = new QVBoxLayout(body);
    body_layout->setContentsMargins(6, 4, 6, 6);
    body_layout->addWidget(widget);
    card_layout->addWidget(body, 1);    // the extra height (resize grip) to the panel
    widget->show();
    // panels made for a column (e.g. from the tabs) need a usable width over the map
    static const QStringList column_panels = {"settings", "flightplan", "checklist", "plotter", "commands"};
    if(widget->sizeHint().width() < 200 || column_panels.contains(id.toLower())) {
        card->setMinimumWidth(420);
    }
    if(!qApp->styleSheet().contains("rail_card")) {
        card->setStyleSheet("#rail_card {background: white; border: 1px solid #c4ccd5; border-radius: 8px;}"
                            "#rail_card_title {font-weight: bold;}"
                            "#rail_card_close {border: 0; background: transparent;}");
    }
    rail_cards[widget] = card;
    card->setProperty("default_min_width", card->minimumWidth());
    new CardMover(header, card, [=,this](QPoint pos) {
        setRailCardPos(widget, pos);
    }, [=,this]() {
        // saved once, after the drag (not at each mouse move)
        emit railCardMoved(widget, card->property("floating").toBool() ? card->pos() : QPoint(-1, -1));
    });
    new CardGrip(card, [=,this](QSize size) {
        setRailCardSize(widget, size);
        emit railCardResized(widget, size);
    });
    if(notifications) {
        notifications->avoid(card);
    }

    card->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    addWidget(card, button, WIDGETS_LEFT, index);
    // panels with unseen content (e.g. chat) show a bubble on their rail button
    if(widget->metaObject()->indexOfSignal("unreadChanged(int)") != -1) {
        connect(widget, SIGNAL(unreadChanged(int)), button, SLOT(setBadge(int)));
    }
    columnLeft->setAlignment(card, Qt::AlignTop | Qt::AlignLeft);   // as tall as its content
    connect(close_button, &QToolButton::clicked, this, [=]() {
        card->hide();
        button->setActive(false);
    });
    connect(button, &LockButton::dragStarted, this, [=,this]() {
        PanelManager::get()->startDrag(id, button, button->grab());
    });
    fitRailCards();
}

int MapWidget::railIndexOf(QWidget* widget) {
    auto card = rail_cards.value(widget, nullptr);
    for(int i=0; i<buttonsLeft.size(); i++) {
        if(card != nullptr && buttonsLeft[i]->widget() == card) {
            return i;
        }
    }
    return -1;
}

int MapWidget::railInsertAt(QPoint global_pos, QRect* line) {
    int index = buttonsLeft.size();
    for(int i=0; i<buttonsLeft.size(); i++) {
        auto b = buttonsLeft[i];
        QRect r(b->mapToGlobal(QPoint(0, 0)), b->size());
        if(global_pos.y() < r.center().y()) {
            index = i;
            break;
        }
    }
    if(line != nullptr && !buttonsLeft.isEmpty()) {
        // between two buttons
        auto ref = buttonsLeft[qMin(index, static_cast<int>(buttonsLeft.size()) - 1)];
        QRect r(ref->mapToGlobal(QPoint(0, 0)), ref->size());
        int y = index < buttonsLeft.size() ? r.top() - 3 : r.bottom() + 1;
        *line = QRect(r.left() - 2, y, r.width() + 4, 4);
    }
    return index;
}

void MapWidget::removeRailPanel(QWidget* widget) {
    auto card = rail_cards.take(widget);
    if(card == nullptr) {
        return;
    }
    for(auto button: QList<LockButton*>(buttonsLeft)) {
        if(button->widget() == card) {
            buttonsLeft.removeAll(button);
            buttonsLeftLayout->removeWidget(button);
            button->hide();
            button->deleteLater();
        }
    }
    columnLeft->removeWidget(card);
    widget->hide();
    widget->setParent(nullptr);     // taken out of the card (the caller puts it elsewhere)
    card->hide();
    card->deleteLater();
}

/// The cards are not taller than the map.
void MapWidget::fitRailCards() {
    for(auto card: rail_cards) {
        QSize user = card->property("user_size").toSize();
        if(user.isValid()) {
            // a saved size is not smaller than the panel needs now
            user = user.expandedTo(card->minimumSizeHint());
            QSize s(user.width(), qMin(user.height(), qMax(card->minimumSizeHint().height(), height() - 30)));
            card->setMinimumSize(s);
            card->setMaximumSize(s);
        } else {
            card->setMaximumHeight(qMax(200, height() - 30));
        }
    }
}

void MapWidget::setRailCardPos(QWidget* widget, QPoint pos) {
    auto card = rail_cards.value(widget, nullptr);
    if(card == nullptr) {
        return;
    }
    bool floating = card->property("floating").toBool();
    if(pos.x() < 0 || pos.y() < 0) {
        // back next to the rail
        card->setProperty("user_pos", QPoint());
        if(floating) {
            card->setProperty("floating", false);
            bool visible = card->isVisible();
            columnLeft->addWidget(card);
            columnLeft->setAlignment(card, Qt::AlignTop | Qt::AlignLeft);
            card->setVisible(visible);
        }
        return;
    }
    if(!floating) {
        columnLeft->removeWidget(card);
        card->setProperty("floating", true);
    }
    // on the map, right of the rail (the position asked is kept, e.g. for a bigger map later)
    card->setProperty("user_pos", pos);
    auto rail = findChild<QWidget*>("map_rail");
    int min_x = rail ? rail->geometry().right() + 4 : 0;
    QPoint p(qBound(min_x, pos.x(), qMax(min_x, width() - card->width())),
             qBound(0, pos.y(), qMax(0, height() - card->height())));
    card->move(p);
    card->raise();
}

void MapWidget::setRailCardSize(QWidget* widget, QSize size) {
    auto card = rail_cards.value(widget, nullptr);
    if(card == nullptr) {
        return;
    }
    card->setProperty("user_size", size);
    if(!size.isValid()) {
        // as big as the panel asks
        card->setMinimumSize(card->property("default_min_width").toInt(), 0);
        card->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        card->adjustSize();
    }
    fitRailCards();
}

void MapWidget::resizeEvent(QResizeEvent *event) {
    Map2D::resizeEvent(event);
    fitRailCards();
    // the cards moved over the map stay on it
    for(auto it = rail_cards.begin(); it != rail_cards.end(); ++it) {
        if(it.value()->property("floating").toBool()) {
            setRailCardPos(it.key(), it.value()->property("user_pos").toPoint());
        }
    }
    placeFrozenBanner();
}

/// Low battery and lost links of an aircraft as notifications at the top of the map.
void MapWidget::watchAircraftStates(QString ac_id) {
    if(!AircraftManager::get()->aircraftExists(ac_id)) {
        return;
    }
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    auto watcher = ac->getStatus()->getWatcher();
    QPointer<MapNotifications> n = notifications;
    connect(watcher, &AircraftWatcher::bat_status, this, [=](AircraftWatcher::BatStatus bs) {
        if(!n || !AircraftManager::get()->aircraftExists(ac_id)) {
            return;
        }
        auto msg = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getMessage("ENGINE_STATUS");
        float bat = 0;
        if(msg) {
            msg->getField("bat", bat);
        }
        QString v = QString("%1 V").arg(bat, 0, 'f', 1);
        switch(bs) {
        case AircraftWatcher::BatStatus::CATASTROPHIC:
        case AircraftWatcher::BatStatus::CRITIC:
            n->setState("bat:" + ac_id, ac_id, "Battery critical: " + v, MapNotifications::ALARM);
            break;
        case AircraftWatcher::BatStatus::LOW:
            n->setState("bat:" + ac_id, ac_id, "Battery low: " + v, MapNotifications::WARNING);
            break;
        case AircraftWatcher::BatStatus::OK:
            n->setState("bat:" + ac_id, ac_id, "", MapNotifications::INFO);
            break;
        }
    });
    connect(watcher, &AircraftWatcher::link_status, this, [=](AircraftWatcher::LinkStatus ls) {
        if(!n) {
            return;
        }
        switch(ls) {
        case AircraftWatcher::LinkStatus::LINK_LOST:
            n->setState("link:" + ac_id, ac_id, "Link lost", MapNotifications::ALARM);
            break;
        case AircraftWatcher::LinkStatus::LINK_PARTIALY_LOST:
            n->setState("link:" + ac_id, ac_id, "Link partly lost", MapNotifications::WARNING);
            break;
        case AircraftWatcher::LinkStatus::LINK_OK:
            n->setState("link:" + ac_id, ac_id, "", MapNotifications::INFO);
            break;
        }
    });
}

bool MapWidget::openRailPanel(QWidget* widget) {
    auto card = rail_cards.value(widget, nullptr);
    if(card == nullptr) {
        return false;
    }
    for(auto button: buttonsLeft) {
        if(button->widget() == card) {
            if(!button->isActive()) {
                button->setActive(true);
                emit button->clicked(true);
            }
            return true;
        }
    }
    return false;
}

/// Freeze (or unfreeze) the waypoints: they cannot be moved; a banner shows it.
void MapWidget::setFrozen(bool frozen) {
    interaction_state = frozen ? PMIS_FROZEN : PMIS_OTHER;
    setEditorMode();
    frozen_banner->setVisible(frozen);
    placeFrozenBanner();
}

void MapWidget::placeFrozenBanner() {
    if(!frozen_banner || !frozen_banner->isVisible()) {
        return;
    }
    frozen_banner->adjustSize();
    int bottom = bottom_tabs && bottom_tabs->isVisible() ? bottom_tabs->y() : height();
    frozen_banner->move((width() - frozen_banner->width()) / 2, bottom - frozen_banner->height() - 14);
    frozen_banner->raise();
}
