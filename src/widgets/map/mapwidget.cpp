#include "mapwidget.h"

MapWidget::MapWidget(QWidget *parent) : Map2D(QString("://tile_sources.xml"), parent)
{
}
