#include "lock_button.h"
#include <QPainter>
#include <QIcon>
#include <QDebug>
#include <QApplication>

static constexpr int LONG_PRESS_MS = 600;

LockButton::LockButton(QIcon icon, QWidget *parent) : QWidget(parent),
    state(IDLE), icon(icon), icon_lock(":/pictures/lock.svg"), icon_unlocked(":/pictures/unlocked.svg"),
    _locked(false), active(false), _widget(nullptr)
{

}

void LockButton::setSize(QSize s) {
    _size = s;
    auto lock_size = QSize(qMax(12, _size.width()/4), qMax(12, _size.height()/4));
    lock_rect = QRect(_size.width() - lock_size.width(), 0, lock_size.width(), lock_size.height());
    icon_margins = QMargins(_size.width()/5, _size.height()/5, _size.width()/5, _size.height()/5);
    setActiveSide(activeSide);
    updateGeometry();   // the size can come from the style sheet after the layout was made
}

void LockButton::setActiveSide(bool activeLeft) {
    activeSide = activeLeft;
    auto pos_active_rect = activeLeft ? 2 : _size.width()-4;
    active_rect = QRect(pos_active_rect, _size.height()/4, 2, _size.height()-2*_size.height()/4);
}

void LockButton::enterEvent(QEvent* e) {
    (void)e;
    state = HOVER;
    repaint();
}

void LockButton::leaveEvent(QEvent* e) {
    (void)e;
    state = IDLE;
    repaint();
}

void LockButton::paintEvent(QPaintEvent* e) {
    (void)e;
    QPainter painter(this);

    switch (state) {
    case IDLE:
        painter.setBrush(QBrush(active ? m_color_active : m_color_idle));
        break;
    case HOVER:
        painter.setBrush(QBrush(m_color_hover));
        break;
    case PRESSED:
        painter.setBrush(QBrush(m_color_pressed));
        break;
    }

    painter.setPen(Qt::NoPen);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawRoundedRect(rect(), 8, 8);

    QRect icon_rect = rect().marginsRemoved(icon_margins);
    icon.paint(&painter, icon_rect);

    // the lock only when locked, or on hover to lock it
    if(_locked) {
        icon_lock.paint(&painter, lock_rect.marginsRemoved(QMargins(1, 1, 1, 1)));
    } else if(state != IDLE) {
        icon_unlocked.paint(&painter, lock_rect.marginsRemoved(QMargins(1, 1, 1, 1)));
    }


}

void LockButton::mousePressEvent(QMouseEvent *e) {
    if(e->button() != Qt::LeftButton) {
        return;
    }

    if(lock_rect.contains(e->pos())) {
        state = IDLE;
    } else {
        state = PRESSED;
    }
    press_time.start();
    press_pos = e->pos();
    dragging = false;

    repaint();
}

void LockButton::mouseMoveEvent(QMouseEvent *e) {
    if((e->buttons() & Qt::LeftButton) && !dragging && !lock_rect.contains(press_pos) &&
       (e->pos() - press_pos).manhattanLength() > QApplication::startDragDistance() * 2) {
        dragging = true;
        press_time.invalidate();
        state = IDLE;
        update();
        emit dragStarted();
    }
}

void LockButton::mouseReleaseEvent(QMouseEvent *e) {
    if(e->button() != Qt::LeftButton) {
        return;
    }
    if(dragging) {
        dragging = false;
        return;
    }

    state = HOVER;

    bool long_press = press_time.isValid() && press_time.elapsed() > LONG_PRESS_MS;
    if(lock_rect.contains(e->pos()) || long_press) {
        _locked = !_locked;
        emit locked(_locked);
        if(long_press && _locked && !active) {
            // lock and open it
            active = true;
            emit clicked(active);
        }

    } else {
        active = !active;
        state = HOVER;
        emit clicked(active);
    }

    repaint();
}

QSize LockButton::sizeHint() const
{
    return minimumSizeHint();
}

QSize LockButton::minimumSizeHint() const
{
    return _size;
}
