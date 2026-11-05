#include "double_slider.h"
#include <QStyleOptionSlider>
#include <QToolTip>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDebug>
#include <math.h>
#include <algorithm>

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
    int nb_steps = ceil((max - min) / step);
    setRange(0, nb_steps);
    setPageStep(bigStep());
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
    }
}

/// A tenth of the range, at least 2 steps
int DoubleSlider::bigStep() const {
    return std::max(2, static_cast<int>(round((maximum() - minimum()) / 10.)));
}

void DoubleSlider::wheelEvent(QWheelEvent* e) {
    int s = (e->modifiers() & Qt::ShiftModifier) ? bigStep() : 1;
    if(e->angleDelta().y() > 0) {
        setValue(value()+s);
    } else {
        setValue(value()-s);
    }
    e->accept();
}

void DoubleSlider::keyPressEvent(QKeyEvent* e) {
    if(e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        emit returnPressed();
        e->accept();
        return;
    }
    if(e->modifiers() & Qt::ShiftModifier) {
        int dir = 0;
        if(e->key() == Qt::Key_Right || e->key() == Qt::Key_Up) {
            dir = 1;
        } else if(e->key() == Qt::Key_Left || e->key() == Qt::Key_Down) {
            dir = -1;
        }
        if(dir != 0) {
            setValue(value() + dir * bigStep());
            e->accept();
            return;
        }
    }
    QSlider::keyPressEvent(e);
}
