#include "airframe.h"
#include <iostream>
#include <algorithm>

const char* getAttribute(XMLElement* ele, string name) {
    const char* result = ele->Attribute(name.c_str());
    if(result == nullptr) {
        transform(name.begin(), name.end(), name.begin(), ::toupper);
        result = ele->Attribute(name.c_str());
    }
    return result;
}

Airframe::Airframe()
{

}


Airframe::Airframe(std::string uri) {
    setlocale(LC_ALL, "C"); // needed for stod() to use '.' as decimal separator instead of ',' (at least in France)
    XMLDocument doc;

    if(uri.substr(0,4) == "file") {
        string path = uri.substr(7, uri.length()-7);
        doc.LoadFile(path.c_str());
    }

    if(doc.Error()) {
        cerr << "Error parsing " << uri << ": " << doc.ErrorStr();
    }
    else {

        XMLElement* air_root = doc.FirstChildElement( "airframe" );
        const char* airframe_name = getAttribute(air_root, "name");
        if(airframe_name != nullptr) {
            name = airframe_name;
        }

        XMLElement* firmware_node = air_root->FirstChildElement( "firmware" );
        const char* firmware_name = getAttribute(firmware_node, "name");
        firmware = firmware_name;

        //cout << "parse " << name << " with firmware " << firmware << endl;


        XMLElement* section_node = air_root->FirstChildElement( "section" );
        while(section_node != nullptr) {
            //cout << endl;
            struct Section section;
            const char* section_name = getAttribute(section_node, "name");
            const char* section_prefix = getAttribute(section_node, "prefix");
            if(section_name != nullptr) {
                section.name = section_name;
                //cout << "Section " << section_name << "  ";
            }
            if(section_prefix != nullptr) {
                section.prefix = section_prefix;
                //cout << "pref=" << section_prefix << "  ";
            }

            XMLElement* define = section_node->FirstChildElement("define");
            while(define != nullptr) {
                struct Define def;

                def.name = getAttribute(define, "name");
                //cout << "  define{" << def.name;
                const char* define_value = getAttribute(define, "value");
                if(define_value != nullptr) {
                    def.value = define_value;
                    //cout << ", " << define_value;
                }
                //cout << "}";
                section.defines.push_back(move(def));
                define = define->NextSiblingElement("define");
            }

            sections.push_back(move(section));
            section_node = section_node->NextSiblingElement("section");
        }


//        if(lat0 == nullptr || lon0 == nullptr || defalt == nullptr) {
//            throw runtime_error("lat0, lon0 or alt not filled!");
//        }

//        XMLElement* wps = fp_root->FirstChildElement("waypoints");

    }
}


string Airframe::getIconName() {
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
                    return stof(d.value);
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
                    return stof(d.value);
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
                    return stof(d.value);
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
