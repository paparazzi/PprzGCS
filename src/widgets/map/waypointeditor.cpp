#include "waypointeditor.h"
#include "srtm_manager.h"
#include "AircraftManager.h"
#include <QDebug>
#include "coordinatestransform.h"

static const QString REF0 = "Flightplan ref0";
static const QString WGS84 = "WGS84";


static QString editorTitle(WaypointItem* wi, const QList<WaypointItem*>& followers) {
    if(followers.isEmpty()) {
        return "Move waypoint " + wi->waypoint()->getName();
    }
    // of several aircraft: their names too
    bool several = false;
    for(auto f: followers) {
        several = several || f->acId() != wi->acId();
    }
    auto label = [=](WaypointItem* w) {
        QString n = w->waypoint()->getName();
        if(several && AircraftManager::get()->aircraftExists(w->acId())) {
            n += " (" + AircraftManager::get()->getAircraft(w->acId())->name() + ")";
        }
        return n;
    };
    QStringList names = {label(wi)};
    for(auto f: followers) {
        names.append(label(f));
    }
    return "Move waypoints " + names.join(", ");
}

WaypointEditor::WaypointEditor(WaypointItem* wi, QString ac_id, QWidget *parent, QList<WaypointItem*> followers) :
    CardDialog(editorTitle(wi, followers),
               AircraftManager::get()->getAircraft(ac_id)->name(), parent,
               AircraftManager::get()->getAircraft(ac_id)->getColor()),
    wi(wi), followers(followers), ac_id(ac_id), original_pos(wi->getOriginalWaypoint()), original_alt(wi->getOriginalWaypoint()->getAlt())
{
    setObjectName("waypoint_editor");
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    auto body = this;
    auto infoLay = this->body();

    // how far it moved, and the reference of the coordinates
    auto moved_row = new QHBoxLayout();
    moved_label = new QLabel(body);
    moved_label->setObjectName("waypoint_editor_moved");
    auto combo = new QComboBox(body);
    combo->setObjectName("waypoint_editor_ref");
    combo->setToolTip("Coordinates in WGS84 or relative to a waypoint");
    combo->addItem(WGS84);
    combo->addItem(REF0);
    for(auto wp: ac->getFlightPlan()->getWaypoints()) {
        combo->addItem(wp->getName());
    }
    moved_row->addWidget(moved_label, 1);
    moved_row->addWidget(combo);
    infoLay->addLayout(moved_row);

    auto coord_row = [&](QString label, QString value, QLabel*& l, QLineEdit*& e) {
        auto row = new QHBoxLayout();
        l = new QLabel(label, body);
        l->setFixedWidth(32);
        e = new QLineEdit(value, body);
        e->setObjectName("waypoint_editor_coord");
        row->addWidget(l);
        row->addWidget(e, 1);
        infoLay->addLayout(row);
    };
    QLabel *axis1_label, *axis2_label;
    QLineEdit *latEdit, *lonEdit;
    coord_row("lat", QString::number(wi->waypoint()->getLat(), 'f', 7), axis1_label, latEdit);
    coord_row("lon", QString::number(wi->waypoint()->getLon(), 'f', 7), axis2_label, lonEdit);

    auto line = new QFrame(body);
    line->setObjectName("waypoint_editor_line");
    line->setFixedHeight(1);
    infoLay->addWidget(line);

    // altitude: value, old value, height above ground
    auto altLay = new QHBoxLayout();
    auto alt_title = new QLabel("alt", body);
    alt_title->setFixedWidth(32);
    altLay->addWidget(alt_title);
    auto altSpin = new QDoubleSpinBox(body);
    altSpin->setObjectName("waypoint_editor_coord");
    altSpin->setMinimum(-100);
    altSpin->setMaximum(5000);
    altSpin->setDecimals(1);
    altSpin->setSuffix(" m");
    altSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    altSpin->setValue(wi->waypoint()->getAlt());
    altLay->addWidget(altSpin);
    // the old altitude next to the field, in a place of its own (always there: nothing moves)
    was_label = new QLabel(body);
    was_label->setObjectName("card_muted");
    was_label->setStyleSheet("font-size: 11px;");
    was_label->setFixedWidth(was_label->fontMetrics().horizontalAdvance("was 0000 m") + 4);
    altLay->addWidget(was_label);
    altLay->addStretch(1);
    agl_label = new QLabel(body);
    altLay->addWidget(agl_label);
    infoLay->addLayout(altLay);

    auto butLay = new QHBoxLayout();
    butLay->setSpacing(6);
    for(int step: {-10, -1, 1, 10}) {
        auto b = new QPushButton(QString("%1%2").arg(step > 0 ? "+" : QString::fromUtf8("\u2212")).arg(abs(step)), body);
        b->setObjectName("waypoint_editor_step");
        butLay->addWidget(b);
        connect(b, &QPushButton::clicked, this, [=]() {
            altSpin->setValue(altSpin->value() + step);
        });
    }
    infoLay->addLayout(butLay);

    auto updateAgl = [=,this]() { updateInfo(); };
    updateInfo();

    auto updateLatLon = [=,this]() {
        bool ok_a = false;
        bool ok_b = false;
        double a = latEdit->text().toDouble(&ok_a);
        double b = lonEdit->text().toDouble(&ok_b);
        if(ok_a && ok_b) {
            if(combo->currentText() == WGS84) {
                wi->waypoint()->setLat(a);
                wi->waypoint()->setLon(b);
                wi->setPosition(Point2DLatLon(a, b));
            }
            else {
                Waypoint* wp;
                if(combo->currentText() == REF0) {
                    wp = wi->waypoint()->getOrigin();
                } else {
                    wp = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan()->getWaypoint(combo->currentText());
                }
                Waypoint::WpFrame frame = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan()->getFrame();
                wi->waypoint()->setRelative(frame, a, b, wp);
                Point2DLatLon geo(wi->waypoint());
                wi->setPosition(geo);
            }



        } else {
            qDebug() << "invalid input!!!";
        }
    };

    connect(latEdit, &QLineEdit::textEdited, [=,this](){
        updateLatLon();
        updateAgl();
    });

    connect(lonEdit, &QLineEdit::textEdited, [=,this](){
        updateLatLon();
        updateAgl();
    });


    connect(altSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), [=,this](double alt){
        wi->waypoint()->setAlt(alt);
        for(auto f: this->followers) {
            f->waypoint()->setAlt(alt);
        }
        updateAgl();
    });


    connect(combo, &QComboBox::currentTextChanged, this, [=,this](const QString &text) {
        if(text == WGS84) {
            axis1_label->setText("lat");
            axis2_label->setText("lon");
            latEdit->setText(QString::number(wi->waypoint()->getLat(), 'f', 7));
            lonEdit->setText(QString::number(wi->waypoint()->getLon(), 'f', 7));
        }
        else {
            axis1_label->setText("x");
            axis2_label->setText("y");

            Waypoint* wp;
            if(text == REF0) {
                //wp = wi->waypoint()->getOrigin();
                wp = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan()->getOrigin();
            } else {
                wp = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan()->getWaypoint(text);
            }

            Waypoint::WpFrame frame = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan()->getFrame();
            double x0, y0, x, y;
            if(frame == Waypoint::LTP) {
                CoordinatesTransform::get()->wgs84_to_ltp(wi->waypoint()->getOrigin(), wp, x0, y0);
                CoordinatesTransform::get()->wgs84_to_ltp(wi->waypoint()->getOrigin(), wi->waypoint(), x, y);
            } else {
                CoordinatesTransform::get()->wgs84_to_relative_utm(wi->waypoint()->getOrigin(), wp, x0, y0);
                CoordinatesTransform::get()->wgs84_to_relative_utm(wi->waypoint()->getOrigin(), wi->waypoint(), x, y);
            }
            latEdit->setText(QString::number(x-x0, 'f', 1));
            lonEdit->setText(QString::number(y-y0,  'f', 1));

        }
    });


    // Cancel (Esc) and Move (Enter)
    auto cancel = addButton("Cancel", NORMAL, 1);
    auto move = addButton("Move", PRIMARY, 2);
    // the focus on Move, not in the lat field: no text cursor there (with an input method, e.g.
    // IBus on Ubuntu, a small window of it flashed next to the card); Enter moves, Esc cancels
    move->setDefault(true);
    move_button = move;
    // numbers only: no input method (its small window flashed next to the card)
    for(auto w: findChildren<QLineEdit*>()) {
        w->setAttribute(Qt::WA_InputMethodEnabled, false);
    }
    for(auto w: findChildren<QAbstractSpinBox*>()) {
        w->setAttribute(Qt::WA_InputMethodEnabled, false);
    }
    connect(move, &QPushButton::clicked, this, [=,this](){
        emit DispatcherUi::get()->move_waypoint_ui(wi->waypoint(), ac_id);
        for(auto f: this->followers) {
            emit DispatcherUi::get()->move_waypoint_ui(f->waypoint(), f->acId());
        }
        accept();
    });
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
}

void WaypointEditor::showEvent(QShowEvent* event) {
    CardDialog::showEvent(event);
    // once shown (showing gives the focus to the first field): on Move, no text cursor in lat
    QTimer::singleShot(0, this, [this]() {
        if(move_button != nullptr) {
            move_button->setFocus(Qt::OtherFocusReason);
        }
    });
}

/// How far and which way it moved, the old altitude and the height above ground.
void WaypointEditor::updateInfo() {
    auto wp = wi->waypoint();
    double distance = 0, azimut = 0;
    CoordinatesTransform::get()->distance_azimut(original_pos, Point2DLatLon(wp), distance, azimut);
    if(distance < 0.5) {
        moved_label->setText("Not moved");
    } else {
        static const char* dirs[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
        int d = static_cast<int>(round(fmod(fmod(azimut, 360.) + 360., 360.) / 45.)) % 8;
        moved_label->setText(QString("Moved %1 m to the %2").arg(distance, 0, 'f', 0).arg(dirs[d]));
    }
    double alt = wp->getAlt();
    was_label->setText(fabs(alt - original_alt) < 0.05 ? QString() : QString("was %1 m").arg(original_alt, 0, 'f', 0));

    auto fp = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan();
    auto ele = SRTMManager::get()->get_elevation(wp->getLat(), wp->getLon());
    double ground = ele ? ele.value() : fp->getGroundAlt();
    double agl = alt - ground;
    double safe = fp->getSecurityHeight() > 0 ? fp->getSecurityHeight() : 20.;
    bool low = agl < safe;
    // AGL above the terrain (SRTM); H above the flight plan ground altitude without SRTM data
    agl_label->setText(QString("%1 %2 m%3").arg(ele ? "AGL" : "H").arg(agl, 0, 'f', 0).arg(low ? " \u00b7 low" : ""));
    agl_label->setToolTip(ele ? "AGL: above the terrain (SRTM)" : "H: above the flight plan ground altitude (no SRTM data here)");
    agl_label->setStyleSheet(QString("font-weight: bold; color: %1;").arg(low ? "#b42318" : "#1f7a3f"));
}
