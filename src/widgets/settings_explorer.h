#ifndef SETTINGS_EXPLORER_H
#define SETTINGS_EXPLORER_H

#include <QWidget>
#include <vector>
#include <memory>
#include "setting_menu.h"

namespace Ui {
class SettingsExplorer;
}

class SettingsExplorer : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsExplorer(QWidget *parent = nullptr);
    ~SettingsExplorer();

protected:
    void paintEvent(QPaintEvent *event);

private:
    void handleNewAC(QString ac_id);
    void updateMenu();
    void handleRowChanged(int row);
    Ui::SettingsExplorer *ui;

    std::shared_ptr<SettingMenu> lastMenu;

    //std::shared_ptr<QMetaObject::Connection> conn;

    std::map<int, QString> ac_ids;
    std::map<int, QColor> colors;
    std::map<QString, std::vector<size_t>> path;
};

#endif // SETTINGS_EXPLORER_H
