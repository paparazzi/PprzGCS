#include "graphlabel.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <algorithm>
#include <math.h>

GraphLabel::GraphLabel(QWidget *parent) : QWidget(parent)
{
    brushTop = QColor("#ffa500");
    brushBottom = QColor("#7ef17e");
    timespan = 6;
    max = 14;
    min = 8;
}

GraphLabel::GraphLabel(double min, double max, QWidget *parent) : QWidget (parent), min(min), max(max),
    unit(""), precision(2), dual_text(false), indicator_angle(0.0)
{
    brushTop = QColor("#ffa500");
    brushBottom = QColor("#7ef17e");
    timespan = 10;
}


void GraphLabel::pushData(double value) {
    data.push_back(std::make_tuple(QTime::currentTime(), value));

    //erase old data
    while(true) {
        auto [t, v] = data.front();
        if(t.addMSecs(static_cast<int>(timespan*1000.)) < QTime::currentTime()) {
            data.pop_front();
        } else {
            break;
        }
    }
    update();
}

void GraphLabel::paintEvent(QPaintEvent *e) {
    (void)e;
    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.setBrush(brushTop);
    p.drawRoundedRect(e->rect(), 5, 5);

    QPolygonF polygon;
    for(auto [t, v]: data) {
        double y = (1 -std::clamp((v - min) / (max - min), 0., 1.)) * e->rect().height();
        double x = (1 - std::clamp(t.msecsTo(QTime::currentTime())/(timespan*1000.), 0., 1.)) * e->rect().width();
        polygon.prepend(QPointF(x, y));
    }
    if(!polygon.isEmpty()) {
        polygon.append(QPointF(0, polygon.last().y()));
    }
    polygon.append(QPointF(0, e->rect().height()));
    polygon.append(QPointF(e->rect().width(), e->rect().height()));
    if(!polygon.isEmpty()) {
        polygon.append(QPointF(e->rect().width(), polygon.first().y()));
    }
    p.setBrush(brushBottom);
    p.drawPolygon(polygon);

    QFont font = p.font();
    font.setPointSize(12);
    font.setBold(true);
    p.setFont(font);
    p.setPen(Qt::black);
    if(!data.isEmpty()) {
        QString txt = QString::number(std::get<1>(data.last()), 'f', precision) + unit;

        if(dual_text) {
            p.drawText(QRectF(0, 0, e->rect().width(), e->rect().height()/2), Qt::AlignCenter, txt);
            p.drawText(QRectF(0, e->rect().height()/2, e->rect().width(), e->rect().height()/2), Qt::AlignCenter, secondary_text);
        } else {
           p.drawText(QRectF(0, 0, e->rect().width(), e->rect().height()), Qt::AlignCenter, txt);
        }
    }

    if(indicator) {
        p.setPen(Qt::black);
        int dx = e->rect().width() - 20;
        int dy = dx * tan(-indicator_angle);
        QPoint a(10, e->rect().height()/2);
        QPoint b(a.x() + dx, a.y()+dy);
        p.drawLine(a, b);
    }

}

QSize GraphLabel::minimumSizeHint() const
{
    return QSize(60, 50);
}
