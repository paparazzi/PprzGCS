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
#include "pprz_dispatcher.h"
#include "maputils.h"

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
        addLayerControl(tp, shown, i);
        i--;
    }

    setZoom(17);
    centerLatLon(Point2DLatLon(43.462344,1.273044));

    connect(
        PprzDispatcher::get(), &PprzDispatcher::gps,
        [=](pprzlink::Message msg) {
        (void) msg;
            qDebug() << "msg recu !";
        }
    );

}

void MapWidget::addLayerControl(QString name, bool initialState, int z) {
    QString path = qApp->property("APP_DATA_PATH").toString() + "/pictures/" + name + ".png";
    QPixmap thumbnail = QPixmap(path);
    MapLayerControl* layer_control = new MapLayerControl(name, thumbnail, initialState, z, layer_tab);
    layer_tab->addLayerControl(layer_control);

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

    connect(
        layer_control, &MapLayerControl::zValueChanged,
        [=](int z) {
            this->setLayerZ(name, z);
        }
    );
}

void MapWidget::setupUi() {
    horizontalLayout = new QHBoxLayout(this);   // main layout
    layer_tab = new LayerTab(this);              // widget for the left tab
    columnLeft = new QVBoxLayout();             // left icons column
    columnRight = new QVBoxLayout();            // right icons column
    spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

    leftScrollArea = new QScrollArea(this);     //scroll area for the left tab
    leftScrollArea->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    leftScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    leftScrollArea->setWidgetResizable(true);
    leftScrollArea->setWidget(layer_tab);
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

    //layoutTabLeft = new QVBoxLayout(widgetTabLeft); // layout for the widgets in the left tab

    // start the application with the tabs closed.
    leftScrollArea->hide();
}

void MapWidget::addItem(MapItem* map_item) {
    map_item->scaleToZoom(zoom(), scaleFactor());
    map_item->add_to_scene(scene());
    _items.append(map_item);
}

void MapWidget::mousePressEvent(QMouseEvent *event) {
    Map2D::mousePressEvent(event);
    if(event->isAccepted()) {
        std::cout << "ACCEPTED" << std::endl;
    } else {
        std::cout << "NOT ACCEPTED" << std::endl;
    }
    if (event->button() == Qt::LeftButton)    // Left button...
    {
      std::cout << "Left button pressed" << std::endl;
    }
    else if (event->button() == Qt::RightButton)   // Right button...
    {
      std::cout << "Right button pressed" << std::endl;
    }
    else if (event->button() == Qt::MidButton)   // Middle button...
    {
      std::cout << "Middle button pressed" << std::endl;
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent *event) {
    Map2D::mouseMoveEvent(event);
    std::cout << "mouse moved" << std::endl;
}

void MapWidget::mouseReleaseEvent(QMouseEvent *event) {
    Map2D::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton)    // Left button...
    {
      std::cout << "Left button released" << std::endl;
    }
    else if (event->button() == Qt::RightButton)   // Right button...
    {
      std::cout << "Right button released" << std::endl;
    }
    else if (event->button() == Qt::MidButton)   // Middle button...
    {
      std::cout << "Middle button released" << std::endl;
    }

}

void MapWidget::wheelEvent(QWheelEvent* event) {
    Map2D::wheelEvent(event);
    for(auto item: _items) {
        item->scaleToZoom(zoom(), scaleFactor());
    }
}
