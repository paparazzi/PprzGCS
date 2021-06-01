#include "airframe.h"
#include <iostream>
#include <algorithm>

QString getAttribute(QDomElement ele, QString name) {
    auto result = ele.attribute(name, "");
    if(result == "") {
        name = name.toUpper();
        result = ele.attribute(name, "");
    }
    return result;
}

Airframe::Airframe(QDomDocument doc, QObject* parent):  QObject(parent), doc(doc)
{
    auto air_root = doc.firstChildElement( "airframe" );
    name = getAttribute(air_root, "name");

    auto firmware_node = air_root.firstChildElement( "firmware" );
    firmware = getAttribute(firmware_node, "name");


    for(auto section_node = air_root.firstChildElement( "section" );
        !section_node.isNull();
        section_node = section_node.nextSiblingElement("section")) {
        //cout << endl;
        struct Section section;
        section.name = getAttribute(section_node, "name");
        section.prefix = getAttribute(section_node, "prefix");


        for(auto define = section_node.firstChildElement("define");
            !define.isNull();
            define = define.nextSiblingElement("define")) {
            struct Define def;

            def.name = getAttribute(define, "name");
            def.value = getAttribute(define, "value");
            section.defines.push_back(std::move(def));

        }

        sections.push_back(std::move(section));

    }
}

QList<Param> Airframe::getParams() {
    QList<Param> params;

    for(auto section=doc.firstChildElement().firstChildElement("section");
        !section.isNull();
        section=section.nextSiblingElement("section")) {
        for(auto define=section.firstChildElement("define");
            !define.isNull();
            define=define.nextSiblingElement("define")) {
            auto full_name = section.attribute("prefix", "") + define.attribute("name");
            auto value = define.attribute("value");
            auto unit = define.attribute("unit", "");
            params.append({full_name, unit, value});
        }
    }
    return params;
}

void Airframe::setParams(QMap<QString, QString> changed_params) {
    for(auto section=doc.firstChildElement().firstChildElement("section");
        !section.isNull();
        section=section.nextSiblingElement("section")) {
        for(auto define=section.firstChildElement("define");
            !define.isNull();
            define=define.nextSiblingElement("define")) {
            auto full_name = section.attribute("prefix", "") + define.attribute("name");

            for(auto it = changed_params.begin(); it!=changed_params.end(); ++it) {
                if(full_name == it.key()) {
                    define.setAttribute("value", it.value());
                }
            }
        }
    }
}

void Airframe::saveSettings(QString filename) {
    QFile file(filename);
    if( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
    {
        qDebug( "Failed to open file for writing." );
        return;
    }
    QTextStream stream( &file );
    stream << doc.toString();
    file.close();
}

QString Airframe::getIconName() {
    for(auto &s: sections) {
        if(s.name == "GCS") {
            for(auto &d: s.defines) {
                if(d.name == "AC_ICON") {
                    return d.value;
                }
            }
        }
    }

    return firmware;
}

float Airframe::getAltShiftPlus() {
    for(auto &s: sections) {
        if(s.name == "GCS") {
            for(auto &d: s.defines) {
                if(d.name == "ALT_SHIFT_PLUS") {
                    return d.value.toDouble();
                }
            }
        }
    }


    if(firmware == "fixedwing") {
        return ALT_PLUS_FW;
    } else if (firmware == "rotorcraft") {
        return ALT_PLUS_RC;
    }

    return 0;
}

float Airframe::getAltShiftPlusPlus() {
    for(auto &s: sections) {
        if(s.name == "GCS") {
            for(auto &d: s.defines) {
                if(d.name == "ALT_SHIFT_PLUS_PLUS") {
                    return d.value.toDouble();
                }
            }
        }
    }

    if(firmware == "fixedwing") {
        return ALT_PLUS_PLUS_FW;
    } else if (firmware == "rotorcraft") {
        return ALT_PLUS_PLUS_RC;
    }

    return 0;
}

float Airframe::getAltShiftMinus() {
    for(auto &s: sections) {
        if(s.name == "GCS") {
            for(auto &d: s.defines) {
                if(d.name == "ALT_SHIFT_MINUS") {
                    return d.value.toDouble();
                }
            }
        }
    }

    if(firmware == "fixedwing") {
        return ALT_MINUS_FW;
    } else if (firmware == "rotorcraft") {
        return ALT_MINUS_RC;
    }

    return 0;
}
