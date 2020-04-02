#ifndef SETTINGS_H
#define SETTINGS_H

#include <memory>
#include <vector>
#include <string>
#include "tinyxml2.h"
#include "setting.h"

using namespace std;
using namespace tinyxml2;

class SettingMenu
{
public:

    struct ButtonGroup {
        string group_name;
        vector<shared_ptr<Setting::StripButton>> buttons;
    };

    SettingMenu();
    SettingMenu(string uri);
    SettingMenu(XMLElement* setel, uint8_t& setting_no);

    vector<shared_ptr<SettingMenu>> getSettingMenus() {return setting_menus;}
    vector<shared_ptr<Setting>> getSettings() {return settings;}
    vector<shared_ptr<Setting>> getAllSettings();
    vector<shared_ptr<ButtonGroup>> getButtonGroups();

    string getName() {return name;}

private:
    void init(XMLElement* setel, uint8_t& setting_no);

    string name;
    vector<shared_ptr<SettingMenu>> setting_menus;
    vector<shared_ptr<Setting>> settings;
};

#endif // SETTINGS_H
