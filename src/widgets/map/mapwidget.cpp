#include "mapwidget.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include "mapitem.h"
#include <iostream>
#include <QPushButton>
#include <QLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QScrollArea>
#include <QDebug>
#include "imagebutton.h"
#include "maplayercontrol.h"
#include <QApplication>

MapWidget::MapWidget(QWidget *parent) : Map2D(QString("://tile_sources.xml"), parent)
{
    setupUi();

    // Create and add button(s) in the left column
    ImageButton* layers_button = new ImageButton(QIcon(":/pictures/map_layers_normal.svg"), QSize(60,60), true);
    layers_button->setHoverIcon(QIcon(":/pictures/map_layers_hover.svg"));
    layers_button->setPressedIcon(QIcon(":/pictures/map_layers_pressed.svg"));
    connect(
        layers_button, &QPushButton::clicked,
        [=]() { leftScrollArea->setVisible(!leftScrollArea->isVisible());}
    );
    columnLeft->addWidget(layers_button);



    setZoom(17);
    centerLatLon(Point2DLatLon(43.462344,1.273044));
    setTilesPath(qApp->property("APP_DATA_PATH").toString() + "/map");
    int i = tileProvidersNames().length();
    for(auto tp: tileProvidersNames() ) {
        bool shown = false;
        if(tp == "Google") {
            toggleTileProvider(tp, true, i, 1);
            shown = true;
        }
        addLayerControl(tp, shown);
        map_layer_controls[tp]->setZValue(i--);
    }

    setZoom(17);
    centerLatLon(Point2DLatLon(43.462344,1.273044));
}

void MapWidget::addLayerControl(QString name, bool initialState) {
    QString path = qApp->property("APP_DATA_PATH").toString() + "/pictures/" + name + ".png";
    QPixmap thumbnail = QPixmap(path);
    MapLayerControl* layer_control = new MapLayerControl(name, thumbnail, initialState, widgetTabLeft);
    map_layer_controls[name] = layer_control;
    layoutTabLeft->addWidget(layer_control);

    connect(
        layer_control, &MapLayerControl::showLayer,
        [=](bool state) {
            this->toggleTileProvider(name, state, layer_control->zValue(), layer_control->opacity());
            this->updateTiles();
        }
    );

    connect(
        layer_control, &MapLayerControl::layerOpacityChanged,
        [=](qreal opacity) {
            this->setLayerOpacity(name, opacity);
        }
    );
}

void MapWidget::setupUi() {
    horizontalLayout = new QHBoxLayout(this);   // main layout
    widgetTabLeft = new QWidget(this);              // widget for the left tab
    columnLeft = new QVBoxLayout();             // left icons column
    columnRight = new QVBoxLayout();            // right icons column
    spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

    leftScrollArea = new QScrollArea(this);     //scroll area for the left tab
    leftScrollArea->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    leftScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    leftScrollArea->setWidgetResizable(true);
    leftScrollArea->setWidget(widgetTabLeft);
    horizontalLayout->addWidget(leftScrollArea);
    // mysterious magic to make the scrollArea semi-transparent
    QPalette pal;
    pal.setColor(QPalette::Window,QColor(74, 169, 228, 100));
    leftScrollArea->setPalette(pal);
    leftScrollArea->setBackgroundRole(QPalette::Window);
    leftScrollArea->widget()->setPalette(pal);
    leftScrollArea->widget()->setBackgroundRole(QPalette::Window);

    horizontalLayout->addLayout(columnLeft);
    horizontalLayout->addItem(spacer);
    horizontalLayout->addLayout(columnRight);

    layoutTabLeft = new QVBoxLayout(widgetTabLeft); // layout for the widgets in the left tab

    // start the application with the tabs closed.
    leftScrollArea->hide();
}

void MapWidget::addItem(QGraphicsItem* graphicItem, Point2DLatLon latlon, int zValue, double zoomFactor) {
    MapItem* map_item = new MapItem(graphicItem, latlon);
    QPointF point = scenePoint(latlon, zoomLevel());
    map_item->setPos(point);
    map_item->setScale(1/scaleFactor());
    scene()->addItem(map_item);
    map_item->setZValue(zValue);
    map_item->setZoomFactor(zoomFactor);
    map_item->scaleToZoom(zoom(), scaleFactor());
    _items.append(map_item);
}

void MapWidget::addCircle(Point2DLatLon latlon, int size, QBrush brush) {
    QGraphicsEllipseItem *circle = new QGraphicsEllipseItem(- size/2,- size/2, size, size);
    circle->setBrush(brush);
     addItem(circle, latlon, 10, 1.15);
}

void MapWidget::mousePressEvent(QMouseEvent *event) {
    Map2D::mousePressEvent(event);
    if(event->buttons() & Qt::RightButton) {
        Point2DLatLon latlon = latlonFromView(event->pos(), zoomLevel());
        addCircle(latlon, 25);
    }
}


void MapWidget::wheelEvent(QWheelEvent* event) {
    Map2D::wheelEvent(event);
    for(auto item: _items) {
        QPointF point = scenePoint(item->position(), zoomLevel());
        item->setPos(point);
        item->scaleToZoom(zoom(), scaleFactor());
    }
}
