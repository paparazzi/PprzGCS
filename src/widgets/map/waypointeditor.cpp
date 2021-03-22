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

    auto combo = new QComboBox(this);
    combo->addItem("WGS84");
    combo->addItem("WGS84 sexa");
    for(auto &wp: AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getWaypoints()) {
        combo->addItem(wp->getName().c_str());
    }
    infoLay->addWidget(combo);
    combo->setEnabled(false);

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


    connect(combo, &QComboBox::currentTextChanged, [=](const QString &text) {
        if(text == "WGS84" || text == "WGS84 sexa") {
            axis1_label->setText("lat");
            axis2_label->setText("lon");
        } else {
            axis1_label->setText("x");
            axis2_label->setText("y");
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
