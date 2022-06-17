#ifndef MAPLAYERCONTROL_H
#define MAPLAYERCONTROL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>
#include "imagebutton.h"

#define STR(x) #x
#define XSTR(x) STR(x)
#define MSG(x) _Pragma (STR(message (x)))

class MapLayerControl : public QWidget
{
    Q_OBJECT
public:
    explicit MapLayerControl(QString name, QPixmap pixmap, bool initialState, int z, QWidget *parent = nullptr);

    void setShowState(bool state);
    void setOpacitySlider(qreal opacity);
    qreal opacity() {return static_cast<qreal>(opacitySlider->value())/opacitySlider->maximum();}
    int zValue() {return z_value;}
    void setZValue(int z);
    QPixmap pixmap() {

#if QT_VERSION > QT_VERSION_CHECK(5, 15, 0)
        return imageLabel->pixmap(Qt::ReturnByValueConstant::ReturnByValue);
#else
        return QPixmap(imageLabel->pixmap());
#endif
    }
    QString name() {return _name;}

    bool operator <(const MapLayerControl& mlc) const
    {
        return (z_value < mlc.z_value);
    }

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

    QString _name;
    bool showState;
    int z_value;
};

#endif // MAPLAYERCONTROL_H
