#include "waypointeditor.h"
#include "srtm_manager.h"
#include "AircraftManager.h"
#include <QDebug>
#include "coordinatestransform.h"



WaypointEditor::WaypointEditor(WaypointItem* wi, QString ac_id, QWidget *parent) : QDialog(parent),
    wi(wi), ac_id(ac_id)
{

    setWindowTitle(QString(wi->waypoint()->getName()));

    auto lay = new QVBoxLayout(this);
    auto infoLay = new QVBoxLayout();
    lay->addLayout(infoLay);

    auto combo = new QComboBox(this);
    combo->addItem("WGS84");
    combo->addItem("WGS84 sexa");
    for(auto &wp: AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getWaypoints()) {
        combo->addItem(wp->getName());
    }
    infoLay->addWidget(combo);

    auto latLay = new QHBoxLayout();
    auto axis1_label = new QLabel("lat", this);
    latLay->addWidget(axis1_label);
    auto latEdit = new QLineEdit(QString::number(wi->waypoint()->getLat()), this);
    latLay->addWidget(latEdit);

    auto lonLay = new QHBoxLayout();
    auto axis2_label = new QLabel("lon", this);
    lonLay->addWidget(axis2_label);
    auto lonEdit = new QLineEdit(QString::number(wi->waypoint()->getLon()), this);
    lonLay->addWidget(lonEdit);

    auto altLay = new QHBoxLayout();
    altLay->addWidget(new QLabel("alt", this));
    auto altSpin = new QDoubleSpinBox(this);
    altSpin->setMinimum(-100);
    altSpin->setMaximum(5000);
    altSpin->setValue(wi->waypoint()->getAlt());
    altLay->addWidget(altSpin);

    auto aglLabel =new QLabel(this);
    altLay->addWidget(aglLabel);

    auto butLay = new QHBoxLayout();
    auto upButton = new QPushButton(this);
    auto downButton = new QPushButton(this);
    upButton->setText("+10");
    downButton->setText("-10");
    butLay->addWidget(downButton);
    butLay->addWidget(upButton);

    infoLay->addLayout(latLay);
    infoLay->addLayout(lonLay);
    infoLay->addLayout(altLay);
    infoLay->addLayout(butLay);

    auto updateAgl = [=]() {
        auto ele = SRTMManager::get()->get_elevation(wi->waypoint()->getLat(), wi->waypoint()->getLon());
        double agl;
        if(ele) {
            agl = wi->waypoint()->getAlt() - ele.value();
            aglLabel->setStyleSheet("color:black");
        } else {
            double ground_alt = AircraftManager::get()->getAircraft(wi->acId()).getFlightPlan().getGroundAlt();
            agl = wi->waypoint()->getAlt() - ground_alt;
            aglLabel->setStyleSheet("color:red");
        }
        aglLabel->setText("AGL: " + QString::number(agl) + "m");
    };


    updateAgl();

    auto updateLatLon = [=]() {
        bool ok_a = false;
        bool ok_b = false;
        double a = latEdit->text().toDouble(&ok_a);
        double b = lonEdit->text().toDouble(&ok_b);
        if(ok_a && ok_b) {
            if(combo->currentText() == "WGS84") {
                wi->waypoint()->setLat(a);
                wi->waypoint()->setLon(b);
                wi->setPosition(Point2DLatLon(a, b));
            }
            else if(combo->currentText() == "WGS84 sexa") {
                //TODO
            } else {
                for(auto &wp: AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getWaypoints()) {
                    if(combo->currentText() == wp->getName()) {
                        double x0, y0;
                        Point2DLatLon geo(0, 0);
                        Waypoint::WpFrame frame = AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getFrame();
                        if(frame == Waypoint::LTP) {
                            CoordinatesTransform::get()->wgs84_to_ltp(wi->waypoint()->getOrigin(), wp, x0, y0);
                            double x = x0 + a;
                            double y = y0 + b;
                            geo = CoordinatesTransform::get()->ltp_to_wgs84(wi->waypoint()->getOrigin(), x, y);
                        } else {
                            CoordinatesTransform::get()->wgs84_to_relative_utm(wi->waypoint()->getOrigin(), wp, x0, y0);
                            double x = x0 + a;
                            double y = y0 + b;
                            geo = CoordinatesTransform::get()->relative_utm_to_wgs84(wi->waypoint()->getOrigin(), x, y);
                        }
                        wi->waypoint()->setLat(geo.lat());
                        wi->waypoint()->setLon(geo.lon());
                        wi->setPosition(geo);
                        break;
                    }
                }
            }



        } else {
            qDebug() << "invalid input!!!";
        }
    };

    connect(latEdit, &QLineEdit::textEdited, [=](){
        updateLatLon();
        updateAgl();
    });

    connect(lonEdit, &QLineEdit::textEdited, [=](){
        updateLatLon();
        updateAgl();
    });


    connect(altSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double alt){
        wi->waypoint()->setAlt(alt);
        updateAgl();
    });

    connect(upButton, &QAbstractButton::clicked, [=]() {
        wi->waypoint()->setAlt(wi->waypoint()->getAlt() + 10);
        altSpin->setValue(wi->waypoint()->getAlt());
        updateAgl();
    });

    connect(downButton, &QAbstractButton::clicked, [=]() {
        wi->waypoint()->setAlt(wi->waypoint()->getAlt() - 10);
        altSpin->setValue(wi->waypoint()->getAlt());
        updateAgl();
    });


    connect(combo, &QComboBox::currentTextChanged, this, [=](const QString &text) {
        if(text == "WGS84") {
            axis1_label->setText("lat");
            axis2_label->setText("lon");
            latEdit->setText(QString::number(wi->waypoint()->getLat()));
            lonEdit->setText(QString::number(wi->waypoint()->getLon()));
        }
        else if(text == "WGS84 sexa") {
            axis1_label->setText("lat");
            axis2_label->setText("lon");
            qDebug() << "TODO sexa";
        } else {
            axis1_label->setText("x");
            axis2_label->setText("y");


            for(auto &wp: AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getWaypoints()) {
                if(text == wp->getName()) {
                    Waypoint::WpFrame frame = AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getFrame();
                    double x0, y0, x, y;
                    if(frame == Waypoint::LTP) {
                        CoordinatesTransform::get()->wgs84_to_ltp(wi->waypoint()->getOrigin(), wp, x0, y0);
                        CoordinatesTransform::get()->wgs84_to_ltp(wi->waypoint()->getOrigin(), wi->waypoint(), x, y);
                    } else {
                        CoordinatesTransform::get()->wgs84_to_relative_utm(wi->waypoint()->getOrigin(), wp, x0, y0);
                        CoordinatesTransform::get()->wgs84_to_relative_utm(wi->waypoint()->getOrigin(), wi->waypoint(), x, y);
                    }
                    latEdit->setText(QString::number(x-x0));
                    lonEdit->setText(QString::number(y-y0));

                    break;
                }
            }



        }
    });


    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    lay->addWidget(buttonBox);

    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, [=](){
        emit DispatcherUi::get()->move_waypoint(wi->waypoint(), ac_id);
        accept();

    });

    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, [=](){
        reject();
    });
}
