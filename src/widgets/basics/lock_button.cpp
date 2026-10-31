#include "lock_button.h"
#include <QPainter>
#include <QIcon>
#include <QDebug>
#include <QApplication>

LockButton::LockButton(QIcon icon, QWidget *parent) : QWidget(parent),
    state(IDLE), icon(icon), icon_lock(":/pictures/lock.svg"), icon_unlocked(":/pictures/unlocked.svg"),
    _locked(false), active(false), _widget(nullptr), _badge(0)
{

}

void LockButton::setSize(QSize s) {
    _size = s;
    auto lock_size = QSize(_size.width()/4, _size.height()/4);
    lock_rect = QRect(_size.width() - lock_size.width(), 0, lock_size.width(), lock_size.height());
    icon_margins = QMargins(_size.width()/20, _size.height()/4, _size.width()/20, _size.height()/20);
    setActiveSide(activeSide);
}

void LockButton::setActiveSide(bool activeLeft) {
    activeSide = activeLeft;
    auto pos_active_rect = activeLeft ? 2 : _size.width()-4;
    active_rect = QRect(pos_active_rect, _size.height()/4, 2, _size.height()-2*_size.height()/4);
}

void LockButton::setBadge(int count) {
    if(count == _badge) {
        return;
    }
    _badge = count;
    update();
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
        painter.setBrush(QBrush(m_color_idle));
        break;
    case HOVER:
        painter.setBrush(QBrush(m_color_hover));
        break;
    case PRESSED:
        painter.setBrush(QBrush(m_color_pressed));
        break;
    }

    painter.setPen(Qt::NoPen);

    painter.drawRect(rect());

    QRect icon_rect = rect().marginsRemoved(icon_margins);
    icon.paint(&painter, icon_rect);

    if(_locked) {
        icon_lock.paint(&painter, lock_rect.marginsRemoved(QMargins(2, 2, 2, 2)));
    } else {
       icon_unlocked.paint(&painter, lock_rect.marginsRemoved(QMargins(2, 2, 2, 2)));
    }

    if(active) {
        painter.setBrush(QBrush(0x50e050));
        painter.drawRect(active_rect);
    }

    if(_badge > 0) {
        // notification bubble in the top-left corner (the lock is top-right)
        QString txt = _badge > 99 ? "99+" : QString::number(_badge);
        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(qMax(8, _size.height()/5));
        painter.setFont(font);
        QFontMetrics fm(font);
        int h = fm.height();
        int w = qMax(h, fm.horizontalAdvance(txt) + h/2);
        QRect badge_rect(1, 1, w, h);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0xe03030));
        painter.drawRoundedRect(badge_rect, h/2.0, h/2.0);
        painter.setPen(Qt::white);
        painter.drawText(badge_rect, Qt::AlignCenter, txt);
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

    repaint();
}

void LockButton::mouseReleaseEvent(QMouseEvent *e) {
    if(e->button() != Qt::LeftButton) {
        return;
    }

    state = HOVER;

    if(lock_rect.contains(e->pos())) {
        _locked = !_locked;
        emit locked(_locked);

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
