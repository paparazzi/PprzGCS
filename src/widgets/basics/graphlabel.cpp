#include "graphlabel.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <algorithm>

GraphLabel::GraphLabel(QWidget *parent) : QWidget(parent)
{
    brushTop = QColor("#ffa500");
    brushBottom = QColor("#7ef17e");
    timespan = 6;
    max = 14;
    min = 8;
}

GraphLabel::GraphLabel(double min, double max, QWidget *parent) : QWidget (parent), min(min), max(max)
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
        QString txt = QString::number(std::get<1>(data.last())) + "V";
        p.drawText(QRectF(0, 0, e->rect().width(), e->rect().height()), Qt::AlignCenter, txt);
    }

}

QSize GraphLabel::minimumSizeHint() const
{
    return QSize(60, 50);
}
