//
// Created by fabien on 22/02/2020.
//

#include "AircraftManager.h"
#include <QDebug>
#include <QApplication>
#include "flightplan.h"
#include "setting_menu.h"
#include "airframe.h"
#include <QSettings>
#include "dispatcher_ui.h"
#include "gcs_utils.h"
#include <optional>

AircraftManager::AircraftManager(PprzApplication* app, PprzToolbox* toolbox) : PprzTool(app, toolbox)
{

}

void AircraftManager::setToolbox(PprzToolbox* toolbox) {
    PprzTool::setToolbox(toolbox);
    connect(PprzDispatcher::get(), &PprzDispatcher::waypoint_moved, this, &AircraftManager::moveWaypoint);
}

Aircraft* AircraftManager::getAircraft(QString id) {
    if(aircrafts.find(id) != aircrafts.end()) {
        return aircrafts[id];
    } else {
        throw runtime_error("No such aircraft!");
    }
}

optional<Aircraft*> AircraftManager::getAircraftByName(QString name) {
    (void)name;
    auto af = std::find_if(aircrafts.begin(), aircrafts.end(),
        [=](Aircraft* ac){
            return ac->name() == name;
        });
    if(af == aircrafts.end()) {
        return nullopt;
    }
    return af.value();
}

QList<Aircraft*> AircraftManager::getAircrafts() {
    return aircrafts.values();
}

void AircraftManager::moveWaypoint(pprzlink::Message msg) {
    QString ac_id;
    uint8_t wp_id = 0;
    float alt, ground_alt;
    msg.getField("ac_id", ac_id);
    msg.getField("wp_id", wp_id);
    double lat = getFloatingField(msg, "lat");
    double lon = getFloatingField(msg, "long");
    msg.getField("alt", alt);
    msg.getField("ground_alt", ground_alt);

    if(AircraftManager::get()->aircraftExists(ac_id) && wp_id != 0) {
        AircraftManager::get()->getAircraft(ac_id)->getStatus()->markWaypointReported(wp_id);
        Waypoint* wp = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan()->getWaypoint(wp_id);
        wp->setLat(lat);
        wp->setLon(lon);
        wp->setAlt(static_cast<double>(alt));
        emit waypoint_changed(wp, ac_id);
    }
}

void AircraftManager::newAircraftConfig(pprzlink::Message msg) {
    QString id, ac_name, default_gui_color, flight_plan, airframe, radio, settings;
    msg.getField("ac_id", id);
    msg.getField("ac_name", ac_name);
    msg.getField("default_gui_color", default_gui_color);
    msg.getField("flight_plan", flight_plan);
    msg.getField("airframe", airframe);
    msg.getField("radio", radio);
    msg.getField("settings", settings);

    QColor color = aircraftColor(id, ac_name, parseColor(default_gui_color));

    if(aircraftExists(id)) {
        qDebug() << "Aircraft " << id << " already exits!";
        return;
    }

   auto config = new ConfigData(id, ac_name, color);

   connect(config, &ConfigData::configReady, this, &AircraftManager::addAircraft);

   config->setFlightPlan(flight_plan);
   config->setAirframe(airframe);
   config->setSettings(settings);
}

void AircraftManager::addAircraft(ConfigData* config) {
    aircrafts[config->getId()] = new Aircraft(config);
    aircrafts[config->getId()]->getStatus()->getWatcher()->init();
    emit DispatcherUi::get()->new_ac_config(config->getId());
    if(select_when_added.remove(config->getId())) {
        emit DispatcherUi::get()->ac_selected(config->getId());
    }
}

void AircraftManager::addFPAircraft(QString ac_id, QString flightplan) {
    static int last_color = (int) Qt::red;
    auto color = QColor((Qt::GlobalColor)last_color++);
    auto config = new ConfigData(ac_id, ac_id, color);
    config->setReal(false);
    QFileInfo fi(flightplan);
    config->setFlightPlan(QString("file://%1").arg(fi.absoluteFilePath()));

    aircrafts[config->getId()] = new Aircraft(config);
    emit DispatcherUi::get()->new_ac_config(config->getId());
    emit DispatcherUi::get()->ac_selected(ac_id);
}

bool AircraftManager::aircraftExists(QString id) {
    return aircrafts.find(id) != aircrafts.end();
}

void AircraftManager::removeAircraft(QString ac_id) {
    if(aircraftExists(ac_id)) {
        emit DispatcherUi::get()->ac_deleted(ac_id);
        auto ac = aircrafts[ac_id];
        aircrafts.remove(ac_id);

        QString ac_selected = "";
        if(aircrafts.size() > 0) {
            ac_selected = aircrafts.firstKey();
        }
        emit DispatcherUi::get()->ac_selected(ac_selected);

        ac->deleteLater();

    }
}


QColor AircraftManager::parseColor(QString str) {
    QColor color = QColor();
    auto settings = getAppSettings();

    if(str[0] == '#' && str.size() == 13) {
        int r = QStringView(str).mid(1, 4).toInt(nullptr, 16) >> 8;
        int g = QStringView(str).mid(5, 4).toInt(nullptr, 16) >> 8;
        int b = QStringView(str).mid(9, 4).toInt(nullptr, 16) >> 8;
        color = QColor(r, g, b);
    } else {
        color = QColor(str);
    }

    if(!color.isValid()) {
        color = QColor(settings.value("aircraft_default_color").toString());
    }

    return color;
}


ConfigData::ConfigData(QString ac_id, QString ac_name, QColor color, QObject* parent) :
    QObject(parent),
    ac_id(ac_id), ac_name(ac_name), color(color), real(true)
{
}

void ConfigData::fetch(QString uri, std::function<void(QByteArray)> callback, std::function<void()> on_error) {
    QString separator = "://";
    int sepi = uri.indexOf(separator);

    if(uri.left(sepi) == "file") {
        QString path = uri.mid(sepi + separator.size());
        if(path == "replay") {
            return;
        }
        QFile f(path);
        if(!f.open(QIODevice::ReadOnly)) {
            throw std::runtime_error("Error while loading file " + path.toStdString());
        }
        auto data = f.readAll();
        f.close();
        callback(data);
    } else if(uri.left(sepi) == "http") {
        auto netacc = new QNetworkAccessManager(this);
        connect(netacc, &QNetworkAccessManager::finished, this, [=,this](QNetworkReply* reply) {
            auto data = reply->readAll();
            if(reply->error() == QNetworkReply::NetworkError::NoError) {
                callback(data);
            } else {
                qDebug() << "Error for" << uri << ":" << reply->error();
                if(on_error) {
                    on_error();
                }
            }
            reply->deleteLater();
        });
        QNetworkRequest request(uri);
        netacc->get(request);
    } else {
        throw std::runtime_error("unknown protocol " + uri.left(sepi).toStdString());
    }
}

void ConfigData::checkComplete() {
    if(isComplete() && !ready_sent) {
        ready_sent = true;
        emit configReady(this);
    }
}

void ConfigData::setData(QDomDocument* doc, QString uri) {
    fetch(uri, [=,this](QByteArray data) {
        doc->setContent(data);
        checkComplete();
    });
}

/**
 * Fetch the files of the top-level <include href="..."/> nodes of the airframe.
 * As in Paparazzi, $AC_ID is replaced by the aircraft id, and a relative href is relative to
 * PAPARAZZI_HOME: the airframe URI up to its "/conf/" (file://$PAPARAZZI_HOME/conf/... or
 * http://server/conf/...).
 */
void ConfigData::loadAirframeIncludes() {
    QStringList hrefs;
    auto root = airframe.firstChildElement();
    for(auto inc=root.firstChildElement("include"); !inc.isNull(); inc=inc.nextSiblingElement("include")) {
        auto href = inc.attribute("href");
        if(!href.isEmpty() && !hrefs.contains(href)) {
            hrefs.append(href);
        }
    }

    int conf_idx = uri_airframe.lastIndexOf("/conf/");
    QString base = conf_idx >= 0 ? uri_airframe.left(conf_idx) : QString();
    QString scheme = uri_airframe.left(uri_airframe.indexOf("://"));

    // count them all first: a file is loaded synchronously
    pending_includes += hrefs.size();
    for(auto &href: hrefs) {
        QString file = href;
        file.replace("$AC_ID", ac_id);
        QString uri;
        if(file.startsWith("/")) {
            uri = scheme == "file" ? "file://" + file : QUrl(uri_airframe).resolved(QUrl(file)).toString();
        } else {
            uri = base + "/" + file;
        }
        try {
            fetch(uri, [=,this](QByteArray data) {
                QDomDocument inc_doc;
                inc_doc.setContent(data);
                airframe_includes[href] = inc_doc;
                airframe_include_uris[href] = uri;
                pending_includes -= 1;
                checkComplete();
            }, [=,this]() {
                pending_includes -= 1;
                checkComplete();
            });
        } catch(std::runtime_error &e) {
            qDebug() << "Airframe include" << uri << "not loaded:" << e.what();
            pending_includes -= 1;
        }
    }
}

void ConfigData::setFlightPlan(QString uri) {
    uri_flight_plan = uri;
    setData(&flight_plan, uri);
}

void ConfigData::setAirframe(QString uri) {
    uri_airframe = uri;
    fetch(uri, [=,this](QByteArray data) {
        airframe.setContent(data);
        loadAirframeIncludes();
        checkComplete();
    });
}

void ConfigData::setSettings(QString uri) {
    setData(&settings, uri);
}

QColor AircraftManager::aircraftColor(QString id, QString name, QColor configured) {
    auto settings = getAppSettings();
    QColor chosen(settings.value("aircraft_colors/" + name).toString());
    if(chosen.isValid()) {
        assigned_colors[id] = chosen;
        configured_colors[id] = configured;
        return chosen;
    }
    // the same as last time (e.g. reloaded after a reboot)
    if(assigned_colors.contains(id) && configured_colors[id] == configured) {
        return assigned_colors[id];
    }
    // two colors that look alike: a close hue (both clearly colored), or close RGB values
    auto close = [](QColor a, QColor b) {
        if(a.hsvSaturation() > 80 && b.hsvSaturation() > 80 && a.value() > 80 && b.value() > 80) {
            int dh = abs(a.hsvHue() - b.hsvHue());
            dh = std::min(dh, 360 - dh);
            return dh < 30;
        }
        int dr = a.red() - b.red(), dg = a.green() - b.green(), db = a.blue() - b.blue();
        return dr * dr + dg * dg + db * db < 100 * 100;
    };
    auto used = [&](QColor c) {
        for(auto it = assigned_colors.begin(); it != assigned_colors.end(); ++it) {
            if(it.key() != id && close(it.value(), c)) {
                return true;
            }
        }
        return false;
    };
    QColor color = configured;
    if(color.isValid() && used(color)) {
        static const QList<QColor> palette = {
            QColor("#2060ff"), QColor("#d92d20"), QColor("#1a7f37"), QColor("#8250df"),
            QColor("#e8590c"), QColor("#0e9fa8"), QColor("#c2185b"), QColor("#7a5c00"),
        };
        for(auto &p: palette) {
            if(!used(p)) {
                color = p;
                break;
            }
        }
    }
    assigned_colors[id] = color;
    configured_colors[id] = configured;
    return color;
}
