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
    explicit MapLayerControl(QString name, QPixmap pixmap, QWidget *parent = nullptr);

    void setShowState(bool state);

    //ImageButton* button() {return show_button;}

signals:
    void showLayer(bool);

public slots:

private:
    void toggleShowState();

    QVBoxLayout* verticalLayout;
    QLabel* nameLabel;
    QLabel* imageLabel;
    QSlider* opacitySlider;
    ImageButton *show_button;

    bool showState;
};

#endif // MAPLAYERCONTROL_H
