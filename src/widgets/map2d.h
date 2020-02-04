#ifndef MAP2D_H
#define MAP2D_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QGraphicsTextItem>

class Map2D : public QGraphicsView
{
    Q_OBJECT
public:
    explicit Map2D(QWidget *parent = nullptr);
    Q_PROPERTY(QPointF _pos READ pos WRITE setPos)

    inline const QPointF &pos() const { return _pos; }
    inline void setPos(const QPointF &pos) { _pos = pos;}
    inline void setLatLon0(qreal lat0, qreal lon0) {latLon0 = QPointF(lat0, lon0);}

signals:

public slots:

protected:
    virtual void wheelEvent(QWheelEvent* event);

private slots:
    void acChanged(int);

private:
//    ///
//    /// \brief latLonOfPos
//    /// \return
//    ///
//    QPointF latLonOfPos();

//    QPointF posOfLatLon(QPointF latlon);

    QGraphicsScene* scene;

    QGraphicsTextItem* current_ac;

    ///
    /// \brief _pos: X/Y position in meters
    ///
    QPointF _pos;

    qint16 tileSize;

    ///
    /// \brief latLon0: GPQ coordinates of the (0,0) pos.
    ///
    QPointF latLon0;
};

#endif // MAP2D_H
