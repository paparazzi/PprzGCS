#include "setting_menu.h"
#include "iostream"
#include <clocale>
#include <assert.h>
#include <algorithm>
#include <map>

SettingMenu::SettingMenu()
{

}


SettingMenu::SettingMenu(QString uri) {
    QDomDocument doc;

    if(uri.mid(0,4) == "file") {
        QString path = uri.mid(7, uri.length()-7);
        if(path == "replay") {
            cout << "not parsing settings: replay!" << endl;
            return;
        }
        QFile f(path);
        if(!f.open(QIODevice::ReadOnly)) {
            throw std::runtime_error("Error while loading setting file");
        }
        doc.setContent(&f);
        f.close();

        auto units_root = doc.firstChildElement("units");
    } else {
        throw std::runtime_error("Not implemented ! " + uri.toStdString());
    }

    auto st_root = doc.firstChildElement( "settings" );
    auto sets = st_root.firstChildElement("dl_settings");
    uint8_t setting_no = 0;
    init(sets, setting_no);

}

SettingMenu::SettingMenu(QDomElement setel, uint8_t& setting_no) {
    init(setel, setting_no);
}

void SettingMenu::init(QDomElement setel, uint8_t& setting_no) {
    name = setel.attribute("name", "");

    for(auto sets = setel.firstChildElement(); !sets.isNull(); sets = sets.nextSiblingElement()) {

        if(sets.tagName() == "dl_settings") {
            shared_ptr<SettingMenu> menu = make_shared<SettingMenu>(sets, setting_no);
            setting_menus.push_back(menu);
        } else if (sets.tagName() ==  "dl_setting") {
            shared_ptr<Setting> setting = make_shared<Setting>(sets, setting_no);
            settings.push_back(setting);
            ++setting_no;
        } else {
            auto msg = "Tag " + sets.tagName() + " unknown for dl_settings!";
            runtime_error(msg.toStdString());
        }

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


    std::map<QString, shared_ptr<ButtonGroup>> groups_map;

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

