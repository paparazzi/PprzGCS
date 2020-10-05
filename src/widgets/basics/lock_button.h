#ifndef LOCK_BUTTON_H
#define LOCK_BUTTON_H

#include <QWidget>
#include <QIcon>
#include <QMouseEvent>
#include <QEvent>
#include <QPaintEvent>

class LockButton : public QWidget
{
    Q_OBJECT
public:
    explicit LockButton(QIcon icon, QWidget *parent = nullptr);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    void setActive(bool a) { active = a; repaint();}
    bool isActive() {return active;}
    bool isLocked(){return _locked;}
    void setWidget(QWidget* w) {_widget = w;}
    QWidget* widget(){return _widget;}
    void setSize(QSize s);
    void setActiveSide(bool activeLeft);

protected:
    virtual void enterEvent(QEvent* e);
    virtual void leaveEvent(QEvent* e);
    virtual void paintEvent(QPaintEvent* e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);

signals:
    void clicked(bool);
    void locked(bool);

public slots:

private:

    enum State {
        IDLE,
        HOVER,
        PRESSED
    };

    State state;

    QSize size;
    QMargins icon_margins;

    QIcon icon;
    QIcon icon_lock;
    QIcon icon_unlocked;

    bool _locked;
    bool active;

    QColor color_idle;
    QColor color_hover;
    QColor color_pressed;

    QRect lock_rect;
    QRect active_rect;

    QWidget* _widget;
};

#endif // LOCK_BUTTON_H
