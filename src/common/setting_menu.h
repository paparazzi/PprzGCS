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
    SettingMenu();
    SettingMenu(string uri);
    SettingMenu(XMLElement* setel, uint8_t& setting_no);

    vector<shared_ptr<SettingMenu>> getSettingMenus() {return setting_menus;}
    vector<shared_ptr<Setting>> getSettings() {return settings;}
    vector<shared_ptr<Setting>> getAllSettings();


private:
    void init(XMLElement* setel, uint8_t& setting_no);

    string name;
    vector<shared_ptr<SettingMenu>> setting_menus;
    vector<shared_ptr<Setting>> settings;
};

#endif // SETTINGS_H
