#include "windindicator.h"
#include <qmath.h>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QIcon>
#include <QWheelEvent>
#include "dispatcher_ui.h"

WindIndicator::WindIndicator(QWidget *parent) : QWidget(parent),
    current_ac_id(""), compass(0), wind_threshold(1),
    speed_unit(MS), rotate_state(IDLE),
    _size(100, 100), m_background_color(70, 200, 255, 100),
    m_arrow_color(Qt::red), m_pen_color(Qt::black)
{
    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this, &WindIndicator::setAC);
}

void WindIndicator::resizeEvent(QResizeEvent *event) {
    (void)event;
    QRegion reg(rect(), QRegion::Ellipse);
    setMask(reg);
}

void WindIndicator::paintEvent(QPaintEvent *event) {
    (void)event;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    int penWidth = 3;

    auto r_out = rect().marginsRemoved(QMargins(penWidth, penWidth, penWidth, penWidth));

    //outer border
    painter.setBrush(QBrush(m_background_color));
    painter.setPen(QPen(m_pen_color, penWidth));
    painter.drawEllipse(r_out);

    int ringWidth = r_out.width()/6;
    auto r_in = r_out.marginsRemoved(QMargins(ringWidth, ringWidth, ringWidth, ringWidth));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(r_in);

    auto tr = r_in.center() + QPoint(1, 1);
    painter.translate(tr);

    double rotation = compass - 90;
    painter.rotate(rotation);

    auto center = QPoint((r_out.width() + r_in.width())/4.0, 0);
    painter.setBrush(QBrush(m_arrow_color));
    painter.setPen(Qt::NoPen);
    int dot_size = (r_out.width() - r_in.width())/4.0;
    north_rect = QRect(center.x()-dot_size, center.y()-dot_size, 2*dot_size, 2*dot_size);
    QPolygon tri;
    tri.append(center + QPoint(-dot_size, -dot_size));
    tri.append(center + QPoint(dot_size, 0));
    tri.append(center + QPoint(-dot_size, dot_size));
    painter.drawPolygon(tri);

    if(wind_data.contains(current_ac_id)) {
        double wind_speed = wind_data[current_ac_id].wind_speed;
        double wind_dir = wind_data[current_ac_id].wind_dir;

        // windsock
        QIcon windsock;
        if(wind_speed > wind_threshold) {
            windsock = QIcon(":/pictures/windsock.svg");
        } else {
            windsock = QIcon(":/pictures/windsock_no_wind.svg");
        }
        int wsWidth = r_in.width()*0.8;
        auto r_ws = QRect(-wsWidth/2, -wsWidth/2, wsWidth, wsWidth);
        painter.rotate(wind_dir + 180);
        windsock.paint(&painter, r_ws, Qt::AlignCenter);

        // text
        painter.resetMatrix();
        painter.setPen(m_pen_color);
        auto font = QFont();
        font.setBold(true);
        painter.setFont(font);
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
        painter.drawText(r_in, Qt::AlignCenter, txt);
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
    auto translation = rect().center() + QPoint(1,1);
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
        auto pos = event->pos() - rect().center();
        double angle = atan2(pos.x(), -pos.y());
        emit requestRotation(qRadiansToDegrees(angle));
    }
}

void WindIndicator::mouseReleaseEvent(QMouseEvent *event) {
    (void)event;
    rotate_state = IDLE;
}

void WindIndicator::wheelEvent(QWheelEvent* event) {
    double rot = compass + event->delta() / 10.0;
    emit requestRotation(rot);
}

QSize WindIndicator::sizeHint() const
{
    return minimumSizeHint();
}

QSize WindIndicator::minimumSizeHint() const
{
    return _size;
}
