#ifndef PFD_H
#define PFD_H

#include <QWidget>
#include "pprz_dispatcher.h"

class Pfd : public QWidget
{
    Q_OBJECT
public:

    struct Eulers {
        float roll;
        float pitch;
        float yaw;
    };

    explicit Pfd(QWidget *parent = nullptr);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

protected:
    void paintEvent(QPaintEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
//    virtual void mouseMoveEvent(QMouseEvent *event);
//    virtual void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);

    void paintPitch(QRect rect, QPointF center);
    void paintRoll(QRect rect, QPointF center);
    void paintYaw(QRect rect, QPointF center);


signals:

public slots:

private:
    enum Axis {
        ROLL,
        PITCH,
        YAW
    };

    void changeCurrentAC(QString id);
    void updateEulers(pprzlink::Message msg);

    QRect placeRect(int i);
    QPointF placeCenter(int i);



    Axis places[3];
    std::map<QString, struct Eulers> eulers;

    QString current_ac;

    int border_stroke;

};

#endif // PFD_H
