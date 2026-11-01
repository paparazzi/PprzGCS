#ifndef PFD_H
#define PFD_H

#include <QWidget>
#include "pprz_dispatcher.h"
#include "configurable.h"

/**
 * Primary flight display of the selected aircraft: an attitude indicator (horizon moving with
 * pitch and roll, pitch ladder, roll scale) and a heading tape, with the values as text.
 */
class Pfd : public QWidget, public Configurable
{
    Q_OBJECT
public:

    struct Eulers {
        float roll;
        float pitch;
        float yaw;
        float agl = 0;      // height above ground
        float alt = 0;      // altitude MSL
        float climb = 0;
        float speed = 0;    // ground speed
        float airspeed = 0;
    };

    explicit Pfd(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    void configure(QDomElement) override {};

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void changeCurrentAC(QString id);
    void updateEulers(pprzlink::Message msg);

    void paintAttitude(QPainter& p, QRectF r, float roll, float pitch, QColor ac_color, bool valid);
    void paintHeading(QPainter& p, QRectF r, float heading, bool valid);
    void paintAltitude(QPainter& p, QRectF r, const Eulers& e, bool valid);
    void paintSpeed(QPainter& p, QRectF r, const Eulers& e, bool valid);

    std::map<QString, struct Eulers> eulers;
    QString current_ac;
};

#endif // PFD_H
