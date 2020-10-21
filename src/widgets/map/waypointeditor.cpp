#include "waypointeditor.h"
#include "srtm_manager.h"
#include "AircraftManager.h"
#include <QDebug>

WaypointEditor::WaypointEditor(WaypointItem* wi, QString ac_id, QWidget *parent) : QDialog(parent),
    wi(wi), ac_id(ac_id)
{

    setWindowTitle(QString(wi->waypoint()->getName().c_str()));

    auto lay = new QVBoxLayout(this);
    auto infoLay = new QVBoxLayout();
    lay->addLayout(infoLay);

    auto latLay = new QHBoxLayout();
    latLay->addWidget(new QLabel("lat", this));
    auto latEdit = new QLineEdit(QString::number(wi->waypoint()->getLat()), this);
    latLay->addWidget(latEdit);

    auto lonLay = new QHBoxLayout();
    lonLay->addWidget(new QLabel("lon", this));
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
    butLay->addWidget(upButton);
    butLay->addWidget(downButton);

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

    infoLay->addLayout(latLay);
    infoLay->addLayout(lonLay);
    infoLay->addLayout(altLay);
    infoLay->addLayout(butLay);

    connect(latEdit, &QLineEdit::textChanged, [=](){
        bool ok = false;
        double lat = latEdit->text().toDouble(&ok);
        if(ok) {
            wi->waypoint()->setLat(lat);
        } else {
            qDebug() << "invalid input!!!";
        }
        updateAgl();
    });

    connect(lonEdit, &QLineEdit::textChanged, [=](){
        bool ok = false;
        double lon = lonEdit->text().toDouble(&ok);
        if(ok) {
            wi->waypoint()->setLon(lon);
        } else {
            qDebug() << "invalid input!!!";
        }
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


    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    lay->addWidget(buttonBox);

    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, [=](){
        emit(DispatcherUi::get()->move_waypoint(wi->waypoint(), ac_id));
        accept();

    });

    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, [=](){
        reject();
    });
}
