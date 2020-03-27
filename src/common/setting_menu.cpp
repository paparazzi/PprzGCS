#include "setting_menu.h"
#include "iostream"
#include <clocale>
#include <assert.h>
#include <algorithm>


SettingMenu::SettingMenu()
{

}


SettingMenu::SettingMenu(std::string uri) {
    setlocale(LC_ALL, "C"); // needed for stod() to use '.' as decimal separator instead of ',' (at least in France)
    cout << uri << endl;
    XMLDocument doc;


    if(uri.substr(0,4) == "file") {
        cout << "It's a file!" << endl;
        string path = uri.substr(7, uri.length()-7);
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
        for(auto sb: setting->getStripButtons()) {
            buttons.push_back(sb);
        }
    }

    sort(buttons.begin(), buttons.end(),
        [](shared_ptr<Setting::StripButton> lb, shared_ptr<Setting::StripButton> rb) {
                return lb->group < rb->group;
    });


    vector<shared_ptr<ButtonGroup>> groups;

    shared_ptr<ButtonGroup> bg = make_shared<ButtonGroup>();
    bg->group_name = buttons[0]->group;
    groups.push_back(bg);

    for(auto b: buttons) {
        if(b->group != groups[groups.size()-1]->group_name) {
            shared_ptr<ButtonGroup> bg = make_shared<ButtonGroup>();
            bg->group_name = b->group;
            groups.push_back(bg);
        }
        groups[groups.size()-1]->buttons.push_back(b);
    }

    return groups;
}

