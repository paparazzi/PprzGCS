#include "colorlabel.h"
#include <QPainter>
#include <QPaintEvent>

ColorLabel::ColorLabel(QWidget *parent) : QWidget(parent),
    text(""), brush(Qt::red), minSize(QSize(50, 18))
{

}


void ColorLabel::paintEvent(QPaintEvent *e) {
    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.setBrush(brush);
    p.drawRect(0, 0, e->rect().width(), e->rect().height());


    QFont font = p.font();
    font.setPointSize(12);
    font.setBold(true);
    p.setFont(font);
    p.setPen(Qt::black);
    p.drawText(QRectF(0, 0, e->rect().width(), e->rect().height()), Qt::AlignCenter, text);
}

QSize ColorLabel::minimumSizeHint() const
{
    return minSize;
}
