#ifndef SETTINGS_VIEWER_H
#define SETTINGS_VIEWER_H

#include <QWidget>
#include <QtWidgets>
#include <QBoxLayout>
#include "configurable.h"
#include <setting_menu.h>
#include <memory>
#include "pprz_dispatcher.h"
#include <QMouseEvent>

class SettingsViewer : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsViewer(QString ac_id, QWidget *parent = nullptr);

signals:

public slots:

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    void init(QString ac_id);
    void create_widgets(shared_ptr<SettingMenu> setting_menu, QList<shared_ptr<SettingMenu>> stack);
    void populate_search_results(QString searched);
    void restore_searched_items();
    QWidget* makeSettingWidget(shared_ptr<Setting> setting, QWidget* parent);
    void updateSettings(QString id, shared_ptr<Setting>, float);

    QString ac_id;
    QVBoxLayout* main_layout;
    QHBoxLayout* search_layout;
    QHBoxLayout* path_layout;
    QStackedWidget* path;
    QScrollArea* scroll;
    QStackedWidget* scroll_content;
    QVBoxLayout* content_layout;

    QToolButton* button_home;

    QLineEdit* search_bar;

    QList<QWidget*> path_widgets;

    QMap<shared_ptr<SettingMenu>, int> widgets_indexes;
    QMap<shared_ptr<SettingMenu>, int> path_indexes;

    int last_widget_index;
    int last_path_index;

    QMap<shared_ptr<Setting>, QWidget*> setting_widgets;
    QMap<shared_ptr<SettingMenu>, QWidget*> setting_menu_widgets;

    int search_result_index;
    int search_path_index;

    struct PositionHistory {
        QWidget* widget;
        QBoxLayout* layout;
        int index;
    };

    QList<PositionHistory> pos_hist;


    std::map<shared_ptr<Setting>, std::function<void(double)>> label_setters;
    std::map<shared_ptr<Setting>, std::function<void(double)>> setters;
    std::map<shared_ptr<Setting>, bool> initialized;

};

#endif // SETTINGS_VIEWER_H
