#include "graphicsline.h"


GraphicsLine::GraphicsLine(QLineF linef, QObject *parent) :
    QObject(parent),
    QGraphicsLineItem (linef)
{

}
