#include "waypointeditor.h"
#include <QDebug>

WaypointEditor::WaypointEditor(WaypointItem* wi, QString ac_id, QWidget *parent) : QDialog(parent),
    wi(wi), ac_id(ac_id)
{

    setWindowTitle(QString(wi->waypoint()->getName().c_str()));

    auto lay = new QVBoxLayout(this);
    auto infoLay = new QHBoxLayout();
    lay->addLayout(infoLay);

    auto latlonLay = new QVBoxLayout();
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

    latlonLay->addLayout(latLay);
    latlonLay->addLayout(lonLay);
    latlonLay->addLayout(altLay);
    infoLay->addLayout(latlonLay);


    connect(latEdit, &QLineEdit::textChanged, [=](){
        bool ok = false;
        double lat = latEdit->text().toDouble(&ok);
        if(ok) {
            wi->waypoint()->setLat(lat);
        } else {
            qDebug() << "invalid input!!!";
        }
    });

    connect(lonEdit, &QLineEdit::textChanged, [=](){
        bool ok = false;
        double lon = lonEdit->text().toDouble(&ok);
        if(ok) {
            wi->waypoint()->setLon(lon);
        } else {
            qDebug() << "invalid input!!!";
        }
    });


    connect(altSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double alt){
        wi->waypoint()->setAlt(alt);
    });


    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    lay->addWidget(buttonBox);

    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, [=](){
        qDebug() << "dialog ok!";
        emit(DispatcherUi::get()->move_waypoint(wi->waypoint(), ac_id));
        accept();

    });

    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, [=](){
        qDebug() << "dialog cancel!";
        reject();
    });

//    connect(bb->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &WaypointEditor::accept);
//    connect(bb->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &WaypointEditor::reject);

}
