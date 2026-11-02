#ifndef SETTINGS_VIEWER_H
#define SETTINGS_VIEWER_H

#include "card_dialog.h"

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
    void create_widgets(SettingMenu* setting_menu, QList<SettingMenu*> stack);
    void populate_search_results(QString searched);
    void restore_searched_items();
    QWidget* makeSettingWidget(Setting* setting, QWidget* parent);
    void updateSettings(QString id, Setting*, float);
    void send(Setting* setting, float value);
    void markPending(Setting* setting, bool is_pending);

    QString ac_id;
    QVBoxLayout* main_layout;
    QHBoxLayout* search_layout;
    QStackedWidget* path;
    QWidget* path_row;
    QScrollArea* scroll;
    QStackedWidget* scroll_content;
    QVBoxLayout* content_layout;

    QToolButton* button_home;
    QToolButton* button_save;

    QLineEdit* search_bar;

    QList<QWidget*> path_widgets;

    QMap<SettingMenu*, int> widgets_indexes;
    QMap<SettingMenu*, int> path_indexes;

    int last_widget_index;
    int root_path_index = -1;
    int last_path_index;

    QMap<Setting*, QWidget*> setting_widgets;
    QMap<SettingMenu*, QWidget*> setting_menu_widgets;

    int search_result_index;
    int search_path_index;

    struct PositionHistory {
        QWidget* widget;
        QBoxLayout* layout;
        int index;
    };

    QList<PositionHistory> pos_hist;


    std::map<Setting*, std::function<void(double)>> label_setters;
    std::map<Setting*, std::function<void(double)>> setters;
    std::map<Setting*, bool> initialized;
    std::map<Setting*, QWidget*> rows;                     // the row of each setting
    std::map<Setting*, QLabel*> aircraft_labels;           // the value on the aircraft, while it differs
    std::map<Setting*, std::function<void(bool)>> set_dirty;   // changed here, not sent yet
    std::map<Setting*, bool> dirty;
    std::map<Setting*, float> aircraft_values;             // last value from the aircraft (displayed unit)
    void showAircraftValue(Setting* setting);
    void fitContent();
    /// ask the aircraft its value (GET_DL_SETTING), again after 3 s without answer (3 times)
    void requestValue(Setting* setting, int attempt = 1);
    void setLabelState(QLabel* l, QString state);   // asking, answered, failed (colors)
    std::map<Setting*, int> asking;                        // the attempt waiting for an answer
    QMap<QObject*, Setting*> name_labels;                  // click: slider, double click: ask the value
    std::map<Setting*, std::function<void()>> toggle_slider;
    bool rows_open = false;     // GCS setting: every row open (slider and buttons), as the old layout
    std::map<Setting*, float> pending;      // sent values not yet confirmed by the aircraft

};

class SettingSaver: public CardDialog
{
    Q_OBJECT
public:
    explicit SettingSaver(QString ac_id, QWidget *parent = nullptr);

signals:

public slots:

private:
    QString ac_id;

};

#endif // SETTINGS_VIEWER_H
