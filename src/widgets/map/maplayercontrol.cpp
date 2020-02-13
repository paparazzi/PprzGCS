#include "maplayercontrol.h"
#include <QPixmap>
#include <QPainter>
#include <QPaintEvent>


MapLayerControl::MapLayerControl(QString name, QPixmap pixmap, QWidget *parent) : QWidget(parent)
{
    verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSpacing(0);

    nameLabel = new QLabel(name);
    verticalLayout->addWidget(nameLabel);
    verticalLayout->setAlignment(nameLabel, Qt::AlignHCenter);
    nameLabel->setMargin(5);

    imageLabel = new QLabel(this);
    verticalLayout->addWidget(imageLabel);
    verticalLayout->setAlignment(imageLabel, Qt::AlignHCenter);
    imageLabel->setPixmap(pixmap);

    show_button = new ImageButton(QIcon(":/pictures/show"), QSize(60, 60), false, imageLabel);
    QVBoxLayout* image_layout = new QVBoxLayout(imageLabel);
    image_layout->addWidget(show_button);
    connect(
        show_button, &QPushButton::clicked,
        [=]() {
            toggleShowState();
        });

    opacitySlider = new QSlider(Qt::Horizontal, this);
    verticalLayout->addWidget(opacitySlider);
    //verticalLayout->setAlignment(opacitySlider, Qt::AlignHCenter);
    //opacitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

}


void MapLayerControl::setShowState(bool state) {
    showState = state;
    if(showState) {
        show_button->setNormalIcon(QIcon(":/pictures/hide"));
    } else {
        show_button->setNormalIcon(QIcon(":/pictures/show"));
    }
}

void MapLayerControl::toggleShowState() {
    if(showState) {
        setShowState(false);
    } else {
        setShowState(true);
    }
    emit(showLayer(showState));
}
