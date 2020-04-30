#include "double_slider.h"
#include <QStyleOptionSlider>
#include <QToolTip>
#include <QWheelEvent>
#include <QDebug>

DoubleSlider::DoubleSlider(Qt::Orientation orientation, QWidget *parent) : QSlider(orientation, parent)
{
    connect(this, &QSlider::valueChanged, this, &DoubleSlider::notifyValueChanged);
}


void DoubleSlider::notifyValueChanged(int value) {
    double doubleValue = value * step + min;
    emit doubleValueChanged(doubleValue);
}


void DoubleSlider::setDoubleRange(double mi, double ma, double sp) {
    min = mi;
    max = ma;
    step = sp;
    int nb_steps = (max - min) / step + 1;
    setRange(0, nb_steps);
    if(nb_steps < 20) {
        setTickPosition(QSlider::TicksBelow);
    }
}

void DoubleSlider::setDoubleValue(double v) {
    setValue((v - min)/step);
}

double DoubleSlider::doubleValue() {
    return value()*step + min;
}

void DoubleSlider::sliderChange(QAbstractSlider::SliderChange change)
{
    QSlider::sliderChange(change);

    if (change == QAbstractSlider::SliderValueChange )
    {
        QStyleOptionSlider opt;
        initStyleOption(&opt);

        QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
        QPoint bottomRightCorner = sr.bottomLeft();

        QToolTip::showText(mapToGlobal( QPoint( bottomRightCorner.x(), bottomRightCorner.y() ) ), QString::number(doubleValue()), this);
    }
}

void DoubleSlider::wheelEvent(QWheelEvent* e) {
    if(e->delta() > 0) {
        setValue(value()+1);
    } else {
        setValue(value()-1);
    }
    e->accept();
}
