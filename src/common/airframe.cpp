#include "airframe.h"
#include <iostream>



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
        const char* airframe_name = air_root->Attribute("name");
        if(airframe_name != nullptr) {
            name = airframe_name;
        }

        XMLElement* firmware_node = air_root->FirstChildElement( "firmware" );
        const char* firmware_name = firmware_node->Attribute("name");
        firmware = firmware_name;

        //cout << "parse " << name << " with firmware " << firmware << endl;


        XMLElement* section_node = air_root->FirstChildElement( "section" );
        while(section_node != nullptr) {
            //cout << endl;
            struct Section section;
            const char* section_name = section_node->Attribute("name");
            const char* section_prefix = section_node->Attribute("prefix");
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

                def.name = define->Attribute("name");
                //cout << "  define{" << def.name;
                const char* define_value = define->Attribute("value");
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
    for(auto s: sections) {
        if(s.name == "GCS") {
            for(auto d: s.defines) {
                if(d.name == "AC_ICON") {
                    return d.value;
                }
            }
        }
    }

    return firmware;
}
