#include "setting_menu.h"
#include "iostream"
#include <clocale>
#include <assert.h>
#include <algorithm>
#include <map>

SettingMenu::SettingMenu()
{

}


SettingMenu::SettingMenu(std::string uri) {
    setlocale(LC_ALL, "C"); // needed for stod() to use '.' as decimal separator instead of ',' (at least in France)
    XMLDocument doc;

    if(uri.substr(0,4) == "file") {
        string path = uri.substr(7, uri.length()-7);
        if(path == "replay") {
            cout << "not parsing settings: replay!" << endl;
            return;
        }
        doc.LoadFile(path.c_str());
    }

    if(doc.Error()) {
        cerr << "Error parsing " << uri << ": " << doc.ErrorStr();
    }
    else {
        XMLElement* st_root = doc.FirstChildElement( "settings" );
        XMLElement* sets = st_root->FirstChildElement("dl_settings");
        uint8_t setting_no = 0;
        init(sets, setting_no);
    }

}

SettingMenu::SettingMenu(XMLElement* setel, uint8_t& setting_no) {
    init(setel, setting_no);
}

void SettingMenu::init(XMLElement* setel, uint8_t& setting_no) {
    const char* name_p = setel->Attribute("name");
    if(name_p != nullptr) {
        name = name_p;
    }

    XMLElement* sets = setel->FirstChildElement();

    while(sets != nullptr) {
        if(strcmp(sets->Name(), "dl_settings") == 0) {
            shared_ptr<SettingMenu> menu = make_shared<SettingMenu>(sets, setting_no);
            setting_menus.push_back(menu);
        } else if (strcmp(sets->Name(), "dl_setting") == 0) {
            shared_ptr<Setting> setting = make_shared<Setting>(sets, setting_no);
            settings.push_back(setting);
            ++setting_no;
        } else {
            string msg = string("Tag ") + sets->Name() + string(" unknown for dl_settings!");
            runtime_error(msg.c_str());
        }

        sets = sets->NextSiblingElement();
    }

}


vector<shared_ptr<Setting>> SettingMenu::getAllSettings() {
    vector<shared_ptr<Setting>> v;
    v.insert(v.end(), settings.begin(), settings.end());

    for(auto menu: setting_menus) {
        vector<shared_ptr<Setting>> sets = menu->getAllSettings();
        v.insert(v.end(), sets.begin(), sets.end());
    }

    return v;
}

vector<shared_ptr<SettingMenu::ButtonGroup>> SettingMenu::getButtonGroups() {

    vector<shared_ptr<Setting::StripButton>> buttons;
    for(auto setting: getAllSettings()) {
        for(auto &sb: setting->getStripButtons()) {
            buttons.push_back(sb);
        }
    }


    std::map<std::string, shared_ptr<ButtonGroup>> groups_map;

    for(auto &b: buttons) {
        if(groups_map.find(b->group) == groups_map.end()) {
            groups_map[b->group] = make_shared<ButtonGroup>();
            groups_map[b->group]->group_name = b->group;
        }
        groups_map[b->group]->buttons.push_back(b);
    }


    vector<shared_ptr<ButtonGroup>> groups;

    for( auto it = groups_map.begin(); it != groups_map.end(); ++it ) {
        groups.push_back( it->second );
    }

    return groups;

}

