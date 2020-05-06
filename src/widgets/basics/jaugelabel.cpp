#include "jaugelabel.h"
#include <QPainter>
#include <QPaintEvent>
#include <algorithm>

JaugeLabel::JaugeLabel(QWidget *parent) : JaugeLabel(0, 1, "%",  parent)
{
}

JaugeLabel::JaugeLabel(double min, double max, QString unit, QWidget *parent) :
    QWidget(parent), min(min), max(max), unit(unit), precision(2)
{
    brushRight = QColor("#ffa500");
    brushLeft = QColor("#7ef17e");
    brushKill = Qt::red;
}

QSize JaugeLabel::minimumSizeHint() const
{
    return QSize(60, 18);
}

void JaugeLabel::paintEvent(QPaintEvent *e) {
    QPainter p(this);
    p.setPen(Qt::NoPen);
    if(status) {
        p.setBrush(brushLeft);
        int x = static_cast<int>(std::clamp((value - min) / (max - min), 0., 1.) * e->rect().width());
        p.drawRect(0, 0, x, e->rect().height());

        p.setBrush(brushRight);
        p.drawRect(x, 0, e->rect().width()-x, e->rect().height());


    } else {
        p.setBrush(brushKill);
        p.drawRect(0, 0, e->rect().width(), e->rect().height());
    }


    QFont font = p.font();
    font.setPointSize(12);
    font.setBold(true);
    p.setFont(font);
    p.setPen(Qt::black);
    QString txt = QString::number(value, 'f', precision) + unit;
    p.drawText(QRectF(0, 0, e->rect().width(), e->rect().height()), Qt::AlignCenter, txt);



}
