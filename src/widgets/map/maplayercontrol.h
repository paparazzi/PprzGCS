#ifndef MAPLAYERCONTROL_H
#define MAPLAYERCONTROL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>
#include "imagebutton.h"

class MapLayerControl : public QWidget
{
    Q_OBJECT
public:
    explicit MapLayerControl(QString name, QPixmap pixmap, bool initialState, QWidget *parent = nullptr);

    void setShowState(bool state);
    void setOpacitySlider(qreal opacity);
    qreal opacity() {return static_cast<qreal>(opacitySlider->value())/opacitySlider->maximum();}
    int zValue() {return z_value;}
    void setZValue(int z);
    const QPixmap* pixmap() {return imageLabel->pixmap();}

    //ImageButton* button() {return show_button;}

signals:
    void showLayer(bool);
    void layerOpacityChanged(qreal);
    void zValueChanged(int);


public slots:

private:
    void toggleShowState();

    QVBoxLayout* verticalLayout;
    QLabel* nameLabel;
    QLabel* imageLabel;
    QSlider* opacitySlider;
    ImageButton *show_button;

    bool showState;
    int z_value;
};

#endif // MAPLAYERCONTROL_H
