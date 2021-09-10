#ifndef WINDINDICATOR_H
#define WINDINDICATOR_H

#include <QWidget>
#include <QMap>

class WindIndicator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int size READ getSizeProperty WRITE setSizeProperty)
    Q_PROPERTY(QColor background_color MEMBER m_background_color)
    Q_PROPERTY(QColor arrow_color MEMBER m_arrow_color)
    Q_PROPERTY(QColor pen_color MEMBER m_pen_color)
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

    struct WindData {
        double wind_dir;
        double wind_speed;
    };

    explicit WindIndicator(QWidget *parent = nullptr);

    void setSizeProperty(int s) {_size = QSize(s,s); update();}
    int getSizeProperty() {return _size.width();}

    void setWindData(QString ac_id, double dir, double speed) {
        wind_data[ac_id] = {dir, speed};
        update();
    }
    void setCompass(double c) {compass = c; update();}
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

private slots:
    void setAC(QString ac_id) {current_ac_id = ac_id; update();}

private:
    bool onNorth(QMouseEvent* event);

    QString current_ac_id;
    double compass;
    QMap<QString, WindData> wind_data;

    double wind_threshold;

    enum SpeedUnit speed_unit;
    enum RotateState rotate_state;

    QRect north_rect;

    QSize _size;
    QColor m_background_color;
    QColor m_arrow_color;
    QColor m_pen_color;
};

#endif // WINDINDICATOR_H
