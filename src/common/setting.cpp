#include "setting.h"
#include "iostream"
#include <sstream>
#include <assert.h>

#include <algorithm>
#include <iterator>

#include "units.h"

Setting::Setting(XMLElement* setel, uint8_t& setting_no) : setting_no(setting_no)
{
    //var, min, max, step
    var = setel->Attribute("var");
    min = stof(setel->Attribute("min"));
    max = stof(setel->Attribute("max"));
    step = stof(setel->Attribute("step"));

    last_set_values[0] = min;
    last_set_values[1] = min;

    //shortname
    const char* shortname_p = setel->Attribute("shortname");
    if(shortname_p != nullptr) {
        shortname = shortname_p;
    }

    //values
    const char* values_p = setel->Attribute("values");
    if(values_p != nullptr) {
        std::stringstream ss(values_p);
        std::string token;
        while (std::getline(ss, token, '|')) {
            values.push_back(token);
        }
    }

    const char* _unit = setel->Attribute("unit");
    if(_unit != nullptr) {
        unit = _unit;
    }
    const char* _alt_unit = setel->Attribute("alt_unit");
    if(_alt_unit != nullptr) {
        alt_unit = _alt_unit;
    }

    const char* _alt_unit_coef = setel->Attribute("alt_unit_coef");
    if(_alt_unit_coef != nullptr) {
        alt_unit_coef = stof(_alt_unit_coef);
    }

    //module, handler, type, persistent, param
    //TODO



    XMLElement* sets = setel->FirstChildElement();

    while(sets != nullptr) {
        if(strcmp(sets->Name(), "key_press") == 0) {
            shared_ptr<KeyPress> kp = make_shared<KeyPress>();
            const char* key = sets->Attribute("key");
            const char* value = sets->Attribute("value");
            assert(key != nullptr);
            assert(value != nullptr);
            kp->key = key;
            kp->value = stof(value);
            key_presses.push_back(kp);
        } else if (strcmp(sets->Name(), "strip_button") == 0) {
            shared_ptr<StripButton> sb = make_shared<StripButton>();
            const char* name = sets->Attribute("name");
            const char* value = sets->Attribute("value");

            assert(name != nullptr);
            assert(value != nullptr);
            sb->name = name;
            sb->value = stof(value);

            const char* group = sets->Attribute("group");
            if(group != nullptr) {
                sb->group = group;
            }

            const char* icon = sets->Attribute("icon");
            if(icon != nullptr) {
                sb->icon = icon;
            }

            sb->setting_no = setting_no;

            strip_buttons.push_back(sb);
        } else {
            string msg = string("Tag ") + sets->Name() + string(" unknown for dl_setting!");
            runtime_error(msg.c_str());
        }

        sets = sets->NextSiblingElement();
    }

}

float Setting::getAltUnitCoef() {
    if(alt_unit_coef.has_value()) {
        return alt_unit_coef.value();
    }
    else if(unit.size() && alt_unit.size()) {
        auto coef = Units::get()->getCoef(unit, alt_unit);
        if(coef.has_value()) {
            return coef.value();
        }
    }
    //no coef found, just use 1
    return 1.f;
}


ostream& operator<<(ostream& os, const Setting& set) {
    os << "{";

    if(set.shortname != "") {
        os << set.shortname;
    } else {
        os << set.var;
    }

    os << " (" << to_string(set.setting_no) << ")";

    if(set.values.size() > 0) {
        os << " [";
        for (auto p = set.values.begin(); p != set.values.end(); ++p) {
            os << *p;
            if (p != set.values.end() - 1) {
                os << ", ";
            }

        }
        os << "]";
    }

    if(set.key_presses.size() > 0) {
        os << ", [";
        for (auto p = set.key_presses.begin(); p != set.key_presses.end(); ++p) {
            os << **p;
            if (p != set.key_presses.end() - 1) {
                os << ", ";
            }

        }
        os << "]";
    }

    if(set.strip_buttons.size() > 0) {
        os << ", [";
        for (auto p = set.strip_buttons.begin(); p != set.strip_buttons.end(); ++p) {
            os << **p;
            if (p != set.strip_buttons.end() - 1) {
                os << ", ";
            }

        }
        os << "]";
    }

    os << "}";

    return os;
}


ostream& operator<<(ostream& os, const Setting::KeyPress& kp) {
    os << kp.key << ": " << kp.value;
    return os;
}

ostream& operator<<(ostream& os, const Setting::StripButton& sb) {
    os << sb.group << "." << sb.name << ": " << sb.value;
    if(sb.icon != "") {
        os << " " << sb.icon;
    }
    return os;
}
