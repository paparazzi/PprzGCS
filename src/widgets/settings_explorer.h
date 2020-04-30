#ifndef SETTINGS_EXPLORER_H
#define SETTINGS_EXPLORER_H

#include <QWidget>
#include <vector>
#include <memory>
#include "setting_menu.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include "double_slider.h"
#include "pprz_dispatcher.h"

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
    void populateTab(QTabWidget* tab, shared_ptr<SettingMenu> settings, QString ac_id);
    void fillSetting(shared_ptr<Setting> setting, QString ac_id, QGridLayout* lay, QWidget* parent);
    void updateSettings(pprzlink::Message msg);
    Ui::SettingsExplorer *ui;

    std::map<int, QString> ac_ids;
    std::map<int, QColor> colors;

    std::map<shared_ptr<Setting>, std::function<void(double)>> label_setters;
    std::map<shared_ptr<Setting>, std::function<void(double)>> setters;
    std::map<shared_ptr<Setting>, bool> initialized;
};

#endif // SETTINGS_EXPLORER_H
