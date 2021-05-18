#ifndef SETTINGS_H
#define SETTINGS_H

#include <memory>
#include <vector>
#include "setting.h"
#include <QtXml>
#include <QObject>

using namespace std;

class SettingMenu: public QObject
{
public:

    struct ButtonGroup {
        QString group_name;
        QList<shared_ptr<Setting::StripButton>> buttons;
    };

    SettingMenu(QDomDocument doc, QObject* parent=nullptr);
    SettingMenu(QDomElement setel, uint8_t& setting_no, QObject* parent=nullptr);

    QList<SettingMenu*> getSettingMenus() {return setting_menus;}
    QList<shared_ptr<Setting>> getSettings() {return settings;}
    QList<shared_ptr<Setting>> getAllSettings();
    QList<shared_ptr<ButtonGroup>> getButtonGroups();

    QString getName() {return name;}

private:
    void init(QDomElement setel, uint8_t& setting_no);

    QString name;
    QList<SettingMenu*> setting_menus;
    QList<shared_ptr<Setting>> settings;
};

#endif // SETTINGS_H
