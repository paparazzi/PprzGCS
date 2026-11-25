#ifndef SETTINGS_H
#define SETTINGS_H

#include <memory>
#include "setting.h"
#include <QtXml>
#include <QObject>
#include <QHash>

using namespace std;

class SettingMenu: public QObject
{
    Q_OBJECT
public:

    struct ButtonGroup {
        QString group_name;
        QList<shared_ptr<Setting::StripButton>> buttons;
    };

    SettingMenu(QDomDocument doc, QObject* parent);
    SettingMenu(QDomElement setel, uint8_t& setting_no, QObject* parent);

    QList<SettingMenu*> getSettingMenus() {return setting_menus;}
    QList<Setting*> getSettings() {return settings;}
    QList<Setting*> getAllSettings() const {return all_settings;}
    /// Look up a setting by its full variable name or number; nullptr when absent.
    Setting* getSetting(const QString& name) const {return settings_by_name.value(name, nullptr);}
    Setting* getSetting(uint8_t number) const {return settings_by_number.value(number, nullptr);}
    QList<shared_ptr<ButtonGroup>> getButtonGroups();

    QString getName() {return name;}

private:
    void init(QDomElement setel, uint8_t& setting_no);
    void mergeSameNames();
    void rebuildIndexes();

    QString name;
    QList<SettingMenu*> setting_menus;
    QList<Setting*> settings;
    QList<Setting*> all_settings;
    QHash<QString, Setting*> settings_by_name;
    QHash<uint8_t, Setting*> settings_by_number;
};

#endif // SETTINGS_H
