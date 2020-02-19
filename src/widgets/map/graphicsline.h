#ifndef GRAPHICSLINE_H
#define GRAPHICSLINE_H

#include <QObject>
#include <QGraphicsLineItem>

class GraphicsLine : public QObject, public QGraphicsLineItem
{
    Q_OBJECT
public:
    explicit GraphicsLine(QLineF linef, QObject *parent = nullptr);

signals:

public slots:
};

#endif // GRAPHICSLINE_H
