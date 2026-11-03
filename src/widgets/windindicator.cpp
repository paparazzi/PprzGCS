#include "windindicator.h"
#include <qmath.h>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QIcon>
#include <QWheelEvent>
#include <QPainterPath>
#include <QSvgRenderer>
#include "dispatcher_ui.h"

WindIndicator::WindIndicator(QWidget *parent) : QWidget(parent),
    current_ac_id(""), compass(0), wind_threshold(1),
    speed_unit(MS), rotate_state(IDLE),
    _size(140, 40), m_background_color(255, 255, 255, 230),
    m_arrow_color(Qt::red), m_pen_color(Qt::black)
{
    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this, &WindIndicator::setAC);
    connect(DispatcherUi::get(), &DispatcherUi::ac_deleted, this, &WindIndicator::removeAC);
    setToolTip("Wind speed (click to change unit)\nCompass: drag or scroll to rotate the map, double click to reset");
    // no background of its own (themes give widgets one): the pill is drawn, anti-aliased
    setAttribute(Qt::WA_NoSystemBackground);
    setStyleSheet("WindIndicator {background: transparent;}");
    setAttribute(Qt::WA_Hover);     // full opacity under the mouse, also without wind
}

bool WindIndicator::event(QEvent* e) {
    if(e->type() == QEvent::HoverEnter || e->type() == QEvent::HoverLeave) {
        update();
    }
    return QWidget::event(e);
}

void WindIndicator::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

void WindIndicator::paintEvent(QPaintEvent *event) {
    (void)event;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    int penWidth = 2;
    // no wind known (yet) and the map not rotated: faded, until the mouse is over it
    bool rotated = fabs(fmod(fmod(compass, 360.) + 360., 360.)) > 0.5 &&
                   fabs(fmod(fmod(compass, 360.) + 360., 360.) - 360.) > 0.5;
    if(!wind_data.contains(current_ac_id) && !rotated && !underMouse()) {
        painter.setOpacity(0.35);
    }

    // pill background, with a soft shadow and a thin border (contrast on any imagery)
    QRectF pill = QRectF(rect()).adjusted(2, 1, -2, -3);
    double rad = pill.height() / 2.0;
    painter.setPen(Qt::NoPen);
    for(int i=3; i>=1; i--) {
        painter.setBrush(QColor(0, 0, 0, 18));
        painter.drawRoundedRect(pill.translated(0, i * 0.7).adjusted(-i * 0.5, -i * 0.3, i * 0.5, i * 0.5), rad, rad);
    }
    painter.setBrush(QBrush(m_background_color));
    painter.setPen(QPen(m_border_color, 1));
    painter.drawRoundedRect(pill, rad, rad);

    // compass on the left: ring with the north marker (drag it to rotate the map)
    auto r_out = compassRect().marginsRemoved(QMargins(penWidth + 4, penWidth + 3, penWidth + 4, penWidth + 5));
    painter.setPen(QPen(m_pen_color, penWidth));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(r_out);

    painter.save();
    painter.translate(QPointF(r_out.center()) + QPointF(0.5, 0.5));
    painter.rotate(compass - 90);
    int dot_size = qMax(3, r_out.width()/10);
    auto center = QPoint(r_out.width()/2 - dot_size, 0);
    north_rect = QRect(center.x()-dot_size, center.y()-dot_size, 2*dot_size, 2*dot_size);
    QPolygon tri;
    tri.append(center + QPoint(-dot_size, -dot_size));
    tri.append(center + QPoint(dot_size, 0));
    tri.append(center + QPoint(-dot_size, dot_size));
    painter.setBrush(QBrush(m_arrow_color));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(tri);
    painter.restore();

    // wind speed on the right, windsock in the compass
    auto r_txt = rect().adjusted(height() + 2, 0, -height()/3, 0);
    auto font = QFont();
    font.setBold(true);
    font.setPixelSize(qMax(10, height()*2/5));
    painter.setFont(font);
    painter.setPen(m_pen_color);

    if(wind_data.contains(current_ac_id)) {
        double wind_speed = wind_data[current_ac_id].wind_speed;
        double wind_dir = wind_data[current_ac_id].wind_dir;

        // the windsock drawn from the SVG at the angle (not a small image rotated: no jagged edges)
        QSvgRenderer windsock(QString(wind_speed > wind_threshold ? ":/pictures/windsock.svg" : ":/pictures/windsock_no_wind.svg"));
        painter.save();
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.translate(QRectF(r_out).center());
        painter.rotate(compass - 90 + wind_dir + 180);
        // with its own proportions (2:1, it was stretched to a square), a bit slimmer
        double wsWidth = r_out.width() * 0.78;
        double wsHeight = wsWidth * 0.4;
        windsock.render(&painter, QRectF(-wsWidth/2, -wsHeight/2, wsWidth, wsHeight));
        painter.restore();

        QString txt;
        switch (speed_unit) {
        case MS:
            txt = QString("%1 m/s").arg(wind_speed, 0, 'f', 1);
            break;
        case KT:
            txt = QString("%1 kt").arg(wind_speed*1.9438, 0, 'f', 1);
            break;
        case KMH:
            txt = QString("%1 km/h").arg(wind_speed*3.6, 0, 'f', 1);
        }
        painter.drawText(r_txt, Qt::AlignVCenter | Qt::AlignRight, txt);
    }
}

void WindIndicator::changeUnit() {
    switch (speed_unit) {
    case MS:
        speed_unit = KT;
        break;
    case KT:
        speed_unit = KMH;
        break;
    case KMH:
        speed_unit = MS;
    }
    update();
}

/// on the compass (the whole circle: easier to grab than the north marker, also with a trackpad)
bool WindIndicator::onNorth(QMouseEvent* event) {
    return compassRect().contains(event->pos());
}

void WindIndicator::mousePressEvent(QMouseEvent *event) {
    if(onNorth(event)) {
        rotate_state = PRESSED;
        auto pos = event->pos() - compassRect().center();
        drag_angle = qRadiansToDegrees(atan2(pos.x(), -pos.y()));
    } else {
        changeUnit();
    }
}

void WindIndicator::mouseDoubleClickEvent(QMouseEvent *event) {
    if(onNorth(event)) {
        emit requestRotation(0);
    }
}

void WindIndicator::mouseMoveEvent(QMouseEvent *event) {
    if(rotate_state == PRESSED) {
        // turn the map by the angle the pointer turned around the compass center
        auto pos = event->pos() - compassRect().center();
        double angle = qRadiansToDegrees(atan2(pos.x(), -pos.y()));
        double d = angle - drag_angle;
        while(d > 180) d -= 360;
        while(d < -180) d += 360;
        drag_angle = angle;
        emit requestRotation(compass + d);
    }
}

void WindIndicator::mouseReleaseEvent(QMouseEvent *event) {
    (void)event;
    rotate_state = IDLE;
}

void WindIndicator::wheelEvent(QWheelEvent* event) {
    double rot = compass + event->angleDelta().y() / 10.0;
    emit requestRotation(rot);
}

QSize WindIndicator::sizeHint() const
{
    return minimumSizeHint();
}

QSize WindIndicator::minimumSizeHint() const
{
    // without wind data only the compass (a circle), else the pill with the wind speed
    if(!wind_data.contains(current_ac_id)) {
        return QSize(_size.height(), _size.height());
    }
    return _size;
}
