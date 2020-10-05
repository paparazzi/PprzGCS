#include "sidepanel.h"
#include <QMouseEvent>
#include <QDebug>

SidePanel::SidePanel(bool rightPanel, QWidget *parent) : QWidget(parent)
{
    layout = new QHBoxLayout(this);
    scroll_content = new QWidget(this);
    scroll_layout = new QVBoxLayout(scroll_content);
    scrollArea = new QScrollArea(this);
    layout->addWidget(scrollArea);
    //scrollArea->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    //scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(scroll_content);
    QPalette pal;
    pal.setColor(QPalette::Window,QColor(74, 169, 228, 100));
    scrollArea->setPalette(pal);
    scrollArea->setBackgroundRole(QPalette::Window);
    scrollArea->widget()->setPalette(pal);
    scrollArea->widget()->setBackgroundRole(QPalette::Window);

    buttons_layout = new QVBoxLayout();
    layout->addLayout(buttons_layout);

//    if(rightPanel) {
//        layout->addLayout(buttons_layout);
//        layout->addWidget(scrollArea);
//    } else {
//        layout->addWidget(scrollArea);
//        layout->addLayout(buttons_layout);
//    }
    if(rightPanel) {
        layout->setDirection(QBoxLayout::RightToLeft);
    }
scrollArea->hide();


}

void SidePanel::addWidget(QWidget* w, QIcon idle_icon, QIcon hover_icon, QIcon pressed_icon) {
    auto button = new ImageButton(idle_icon, QSize(60,60), true);
    if(!hover_icon.isNull()) {
        button->setHoverIcon(hover_icon);
    }

    if(!pressed_icon.isNull()) {
        button->setPressedIcon(pressed_icon);
    }

    buttons.push_back(button);
    buttons_layout->addWidget(button);

    scroll_layout->addWidget(w);
    widgets.push_back(w);

    connect(
        button, &QPushButton::clicked,
        [=]() {
            bool visible = false;
            for(auto widget: widgets) {
                if(widget != w) {
                    widget->hide();
                } else {
                    visible = widget->isVisible();
                    widget->setVisible(!widget->isVisible());
                }
            }
            scrollArea->setVisible(!visible);
        }
    );

}


void SidePanel::mousePressEvent(QMouseEvent *event) {
    //QWidget::mousePressEvent(event);
    //qDebug() << event;
    //event->ignore();
    event->setAccepted(false);
    QWidget::mousePressEvent(event);

}

bool SidePanel::event(QEvent * e) {
    (void)e;
    return true;
}

void SidePanel::resizeEvent(QResizeEvent *event) {
    (void)event;
//    QRegion reg;
//    for(auto b: buttons){
////        b->rect()
////        QRect rect = placeRect(i);
////        rect.setWidth(rect.width()+border_stroke);
////        rect.setHeight(rect.height()+border_stroke);
////        QPointF center = placeCenter(i);
////        QPoint p(static_cast<int>(center.x()), static_cast<int>(center.y()));
////        rect.moveCenter(p);
//        qDebug() << "butoon  " << b->rect() << "   " << b->pos();
//        QRect rect = b->rect();
//        rect.moveTo(b->pos());
//        QRegion ri(b->rect(), QRegion::Ellipse);
//        reg = reg.united(ri);
//    }

//    setMask(reg);
}


void SidePanel::mouseMoveEvent(QMouseEvent *event) {
    event->setAccepted(false);
    QWidget::mouseMoveEvent(event);
    //event->ignore();
}

void SidePanel::mouseReleaseEvent(QMouseEvent *event) {
    event->setAccepted(false);
    QWidget::mouseReleaseEvent(event);
    //event->ignore();
}
