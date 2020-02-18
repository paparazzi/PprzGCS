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
        qDebug() << "scene pressed " << mouseEvent << "  " << mouseEvent->isAccepted();
    }
}

void MapScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    QGraphicsScene::mouseMoveEvent(mouseEvent);
    if(!mouseEvent->isAccepted()) {
        qDebug() << "scene moved " << mouseEvent << "  " << mouseEvent->isAccepted();
    }
}

void MapScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    QGraphicsScene::mouseReleaseEvent(mouseEvent);
    if(!mouseEvent->isAccepted()) {
        qDebug() << "scene release " << mouseEvent << "  " << mouseEvent->isAccepted();
        if(mouseEvent->button() == Qt::RightButton) {
            emit(rightClick(mouseEvent));
        }
    }
}

void MapScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent) {

}
