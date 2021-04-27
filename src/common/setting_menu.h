#ifndef SETTINGS_H
#define SETTINGS_H

#include <memory>
#include <vector>
#include "setting.h"
#include <QtXml>

using namespace std;

class SettingMenu
{
public:

    struct ButtonGroup {
        QString group_name;
        vector<shared_ptr<Setting::StripButton>> buttons;
    };

    SettingMenu();
    SettingMenu(QString uri);
    SettingMenu(QDomElement setel, uint8_t& setting_no);

    vector<shared_ptr<SettingMenu>> getSettingMenus() {return setting_menus;}
    vector<shared_ptr<Setting>> getSettings() {return settings;}
    vector<shared_ptr<Setting>> getAllSettings();
    vector<shared_ptr<ButtonGroup>> getButtonGroups();

    QString getName() {return name;}

private:
    void init(QDomElement setel, uint8_t& setting_no);

    QString name;
    vector<shared_ptr<SettingMenu>> setting_menus;
    vector<shared_ptr<Setting>> settings;
};

#endif // SETTINGS_H
