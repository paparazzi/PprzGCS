#include "mapscene.h"
#include <QDebug>

MapScene::MapScene(QObject *parent) : QGraphicsScene(parent)
{
}


MapScene::MapScene(qreal x, qreal y, qreal width, qreal height, QObject *parent) :
    QGraphicsScene(x, y, width, height, parent)
{
}


MapScene::MapScene(const QRectF &sceneRect, QObject *parent) :
    QGraphicsScene(sceneRect, parent)
{
}


void MapScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    QGraphicsScene::mousePressEvent(mouseEvent);
    if(!mouseEvent->isAccepted()) {
        emit(eventScene(FPEE_SC_PRESS, mouseEvent));
    }
}

void MapScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    QGraphicsScene::mouseMoveEvent(mouseEvent);
    if(!mouseEvent->isAccepted()) {
        emit(eventScene(FPEE_SC_MOVE, mouseEvent));
    }
}

void MapScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    QGraphicsScene::mouseReleaseEvent(mouseEvent);
    if(!mouseEvent->isAccepted()) {
            emit(eventScene(FPEE_SC_RELEASE, mouseEvent));
    }
}

void MapScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    QGraphicsScene::mouseDoubleClickEvent(mouseEvent);
    if(!mouseEvent->isAccepted()) {
            emit(eventScene(FPEE_SC_DOUBLE_CLICK, mouseEvent));
    }
}
