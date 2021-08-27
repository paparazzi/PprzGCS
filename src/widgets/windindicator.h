#ifndef WINDINDICATOR_H
#define WINDINDICATOR_H

#include <QWidget>

class WindIndicator : public QWidget
{
    Q_OBJECT
public:

    enum SpeedUnit {
        MS,
        KT,
        KMH,
    };

    enum RotateState {
        IDLE,
        PRESSED,
    };

    explicit WindIndicator(QWidget *parent = nullptr);

    void setCompass(double c) {compass = c; update();}
    void setWindDir(double dir) {wind_dir = dir; update();}
    void setWindSpeed(double speed) {wind_speed = speed; update();}
    void setThreshold(double t) {wind_threshold = t; update();}
    void changeUnit();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void requestRotation(double);

protected:
    void paintEvent(QPaintEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    bool onNorth(QMouseEvent* event);

    double compass;
    double wind_dir;
    double wind_speed;

    double wind_threshold;

    enum SpeedUnit speed_unit;
    enum RotateState rotate_state;

    QRect north_rect;

    QPoint last_pos;


};

#endif // WINDINDICATOR_H
