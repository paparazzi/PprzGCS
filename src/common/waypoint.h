#ifndef WAYPOINT_H
#define WAYPOINT_H

#include <memory>
#include <iostream>
#include <map>
#include <QtXml>
#include <memory>

class Waypoint
{
public:

    enum WpFrame {
        WGS84,
        UTM,
        LTP
    };

    enum WpAltType {
        ALT,
        HEIGHT
    };

    Waypoint(QString name, uint8_t id);
    Waypoint(QString name, uint8_t id, double lat, double lon, double alt);
    Waypoint(QDomElement wp, uint8_t wp_id, std::shared_ptr<Waypoint> orig, double defaultAlt, WpFrame frame_type);

    uint8_t getId() const {return id;}
    double getLat() const;
    double getLon() const;
    void setLat (double lat);
    void setLon(double lon);
    void setAlt(double alt) {this->alt = alt;}
    double getAlt() const {return alt;}
    WpFrame getType() const {return type;}
    QString getName() const {return name;}
    std::shared_ptr<Waypoint> getOrigin() {return origin;}
    std::map<QString, QString> getXmlAttributes() { return xml_attibutes;}

    friend std::ostream& operator<<(std::ostream& os, const Waypoint& wp);

private:
    WpFrame type;

    uint8_t id;

    double lat;
    double lon;

    std::shared_ptr<Waypoint> origin;

    double alt;
    WpAltType alt_type;

    QString name;

    std::map<QString, QString> xml_attibutes;
};


#endif // WAYPOINT_H
