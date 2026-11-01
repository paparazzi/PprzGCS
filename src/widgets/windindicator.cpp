#include "windindicator.h"
#include <qmath.h>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QIcon>
#include <QWheelEvent>
#include <QPainterPath>
#include "dispatcher_ui.h"

WindIndicator::WindIndicator(QWidget *parent) : QWidget(parent),
    current_ac_id(""), compass(0), wind_threshold(1),
    speed_unit(MS), rotate_state(IDLE),
    _size(140, 40), m_background_color(255, 255, 255, 230),
    m_arrow_color(Qt::red), m_pen_color(Qt::black)
{
    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this, &WindIndicator::setAC);
    connect(DispatcherUi::get(), &DispatcherUi::ac_deleted, this, &WindIndicator::removeAC);
    setToolTip("Wind speed (click to change unit)\nCompass: drag the north marker or scroll to rotate the map, double-click it to reset");
}

void WindIndicator::resizeEvent(QResizeEvent *event) {
    (void)event;
    QPainterPath path;
    path.addRoundedRect(rect(), height()/2.0, height()/2.0);
    setMask(QRegion(path.toFillPolygon().toPolygon()));
}

void WindIndicator::paintEvent(QPaintEvent *event) {
    (void)event;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    int penWidth = 2;

    // pill background
    painter.setBrush(QBrush(m_background_color));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), height()/2.0, height()/2.0);

    // compass on the left: ring with the north marker (drag it to rotate the map)
    auto r_out = compassRect().marginsRemoved(QMargins(penWidth, penWidth, penWidth, penWidth));
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

        QIcon windsock;
        if(wind_speed > wind_threshold) {
            windsock = QIcon(":/pictures/windsock.svg");
        } else {
            windsock = QIcon(":/pictures/windsock_no_wind.svg");
        }
        painter.save();
        painter.translate(QPointF(r_out.center()) + QPointF(0.5, 0.5));
        painter.rotate(compass - 90 + wind_dir + 180);
        int wsWidth = r_out.width()*0.6;
        windsock.paint(&painter, QRect(-wsWidth/2, -wsWidth/2, wsWidth, wsWidth), Qt::AlignCenter);
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

bool WindIndicator::onNorth(QMouseEvent* event) {
    auto tr = QTransform();
    auto translation = QPointF(compassRect().center()) + QPointF(0.5, 0.5);
    tr.translate(translation.x(), translation.y());
    tr.rotate(compass - 90);
    return tr.mapRect(north_rect).contains(event->pos());
}

void WindIndicator::mousePressEvent(QMouseEvent *event) {
    if(onNorth(event)) {
        rotate_state = PRESSED;
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
        auto pos = event->pos() - compassRect().center();
        double angle = atan2(pos.x(), -pos.y());
        emit requestRotation(qRadiansToDegrees(angle));
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
