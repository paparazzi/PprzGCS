#ifndef SIDEPANEL_H
#define SIDEPANEL_H

#include <QWidget>
#include <QScrollArea>
#include <QHBoxLayout>
#include <imagebutton.h>

class SidePanel : public QWidget
{
    Q_OBJECT
public:
    explicit SidePanel(bool rightPanel = false, QWidget *parent = nullptr);
    void addWidget(QWidget* w, QIcon idle_icon, QIcon hover_icon = QIcon(), QIcon pressed_icon = QIcon());
    QScrollArea* getScrollArea() {return scrollArea;}

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual bool event(QEvent * e);
signals:

public slots:

private:
    QHBoxLayout* layout;
    QScrollArea* scrollArea;
    QWidget* scroll_content;
    QVBoxLayout* scroll_layout;
    QVBoxLayout* buttons_layout;
    std::vector<ImageButton*> buttons;
    std::vector<QWidget*> widgets;
};

#endif // SIDEPANEL_H
