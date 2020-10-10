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
    Q_PROPERTY(QColor color_hover WRITE setBackgroundColorHover DESIGNABLE true)
    Q_PROPERTY(QColor color_idle WRITE setBackgroundColor DESIGNABLE true)
    Q_PROPERTY(QColor color_pressed WRITE setBackgroundColorPressed DESIGNABLE true)
    Q_PROPERTY(int size WRITE setSizeProperty DESIGNABLE true)
public:
    explicit LockButton(QIcon icon, QWidget *parent = nullptr);

    void setBackgroundColorHover ( QColor c ) {_color_hover = c;}
    void setBackgroundColor ( QColor c ){_color_idle = c;}
    void setBackgroundColorPressed ( QColor c ){_color_pressed = c;}
    void setSizeProperty(int s) {setSize(QSize(s,s));}


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

    QSize _size;
    QMargins icon_margins;

    QIcon icon;
    QIcon icon_lock;
    QIcon icon_unlocked;

    bool _locked;
    bool active;

    QColor _color_idle;
    QColor _color_hover;
    QColor _color_pressed;

    QRect lock_rect;
    QRect active_rect;

    QWidget* _widget;
};

#endif // LOCK_BUTTON_H
