#include "maplayercontrol.h"
#include <QPixmap>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>


/**
 * A map layer: thumbnail, name, a switch to show it, and its opacity (when shown).
 * Drag a layer (in the LayerCombo) to change the order.
 */
MapLayerControl::MapLayerControl(QString name, QPixmap pixmap, bool initialState, int z, QWidget *parent) : QWidget(parent),
    _name(name), showState(initialState), z_value(z)
{
    setObjectName("layer_row");
    setAttribute(Qt::WA_StyledBackground);
    auto row = new QHBoxLayout(this);
    row->setContentsMargins(6, 4, 12, 4);     // room from the card edge and the scroll bar
    row->setSpacing(10);

    // rounded thumbnail
    QSize thumb_size(36, 26);
    QPixmap thumb(thumb_size * devicePixelRatioF());
    thumb.setDevicePixelRatio(devicePixelRatioF());
    thumb.fill(Qt::transparent);
    {
        QPainter p(&thumb);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath path;
        path.addRoundedRect(QRectF(0, 0, thumb_size.width(), thumb_size.height()), 4, 4);
        p.setClipPath(path);
        p.drawPixmap(QRect(QPoint(0, 0), thumb_size), pixmap.scaled(thumb_size * devicePixelRatioF(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }
    imageLabel = new QLabel(this);
    imageLabel->setPixmap(thumb);
    imageLabel->setFixedSize(thumb_size);
    row->addWidget(imageLabel);

    auto col = new QVBoxLayout();
    col->setSpacing(4);
    nameLabel = new QLabel(name, this);
    nameLabel->setObjectName("layer_name");
    col->addWidget(nameLabel);
    // the opacity: hidden until the button next to the switch is pressed
    opacitySlider = new QSlider(Qt::Horizontal, this);
    opacitySlider->setToolTip("Opacity");
    opacitySlider->setValue(opacitySlider->maximum());
    opacitySlider->hide();
    col->addWidget(opacitySlider);
    row->addLayout(col, 1);

    auto opacity_button = new QToolButton(this);
    opacity_button->setObjectName("layer_opacity_button");
    opacity_button->setText(QString::fromUtf8("\xE2\x97\x90"));     // half circle
    opacity_button->setToolTip("Opacity");
    opacity_button->setCheckable(true);
    row->addWidget(opacity_button);
    connect(opacity_button, &QToolButton::toggled, opacitySlider, &QWidget::setVisible);

    show_switch = new Switch(this);
    show_switch->setToolTip("Show this layer");
    row->addWidget(show_switch);
    setShowState(initialState);

    connect(show_switch, &QAbstractButton::clicked, this, [=,this]() {
        toggleShowState();
    });

    connect(
        opacitySlider, &QSlider::valueChanged,
        [=,this](int value) {
            (void) value;
            emit layerOpacityChanged(opacity());
        });
}


void MapLayerControl::setShowState(bool state) {
    showState = state;
    show_switch->setChecked(state);
    opacitySlider->setEnabled(state);
    nameLabel->setEnabled(state);
}

void MapLayerControl::setOpacitySlider(qreal opacity) {
    opacity = qMax(0., qMin(opacity, 1.));
    int value = static_cast<int>(opacity * opacitySlider->maximum());
    opacitySlider->setValue(value);
}

void MapLayerControl::toggleShowState() {
    if(showState) {
        setShowState(false);
    } else {
        setShowState(true);
    }
    emit showLayer(showState);
}

void MapLayerControl::setZValue(int z) {
    z_value = z;
    emit zValueChanged(z);
}

