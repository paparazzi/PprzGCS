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

Airframe::Airframe()
{

}


Airframe::Airframe(QString uri) {

    QDomDocument doc;

    if(uri.mid(0,4) == "file") {
        QString path = uri.mid(7, uri.length()-7);
        QFile f(path);
        if(!f.open(QIODevice::ReadOnly)) {
            throw std::runtime_error("Error while loading airframe file");
        }
        doc.setContent(&f);
        f.close();
    } else {
        throw std::runtime_error("Not implemented ! " + uri.toStdString());
    }

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
            section.defines.push_back(move(def));

        }

        sections.push_back(move(section));

    }

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
