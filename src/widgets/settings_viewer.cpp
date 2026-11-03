#include "settings_viewer.h"
#include "dispatcher_ui.h"
#include "pprz_dispatcher.h"
#include "AircraftManager.h"
#include "double_slider.h"
#include "switch.h"
#include "gcs_utils.h"
#include <QDebug>

SettingsViewer::SettingsViewer(QString ac_id, QWidget *parent) : QWidget(parent), ac_id(ac_id)
{
    main_layout = new QVBoxLayout(this);
    search_layout = new QHBoxLayout();
    main_layout->addItem(search_layout);


    button_home = new QToolButton(this);
    button_home->setObjectName("setting_tool");
    button_home->setIcon(QIcon(user_or_app_path("pictures/tool_home.svg")));
    button_home->setIconSize(QSize(20, 20));

    button_save = new QToolButton(this);
    button_save->setObjectName("setting_tool");
    button_save->setIcon(QIcon(user_or_app_path("pictures/tool_save.svg")));
    button_save->setIconSize(QSize(20, 20));

    // home, search and save on one row; the path (System > Logger) below, only inside a group
    main_layout->setContentsMargins(6, 6, 6, 6);
    // the path row (inside a group): home, then the groups (System > Logger)
    path_row = new QWidget(this);
    auto path_row_layout = new QHBoxLayout(path_row);
    path_row_layout->setContentsMargins(0, 0, 0, 0);
    path_row_layout->setSpacing(2);
    path = new QStackedWidget(this);
    path->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    path_row_layout->addWidget(button_home);
    path_row_layout->addWidget(path, 1);
    main_layout->addWidget(path_row);
    button_home->setToolTip("All groups");
    button_save->setToolTip("Save the settings in the airframe file");

    scroll = new QScrollArea();
    scroll_content = new QStackedWidget();
    scroll_content->setContentsMargins(0, 0, 4, 0);     // next to the scroll bar

    main_layout->addWidget(scroll, 1);    // the list takes the extra height (e.g. in a tab)
    scroll->setWidget(scroll_content);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // The size of the list is the one of the group shown, not the biggest of all groups: no empty
    // space to scroll down to, and no sideways scrolling (the hidden groups are ignored).
    connect(scroll_content, &QStackedWidget::currentChanged, this, [=,this](int index) {
        for(int i=0; i<scroll_content->count(); i++) {
            auto w = scroll_content->widget(i);
            if(i == index) {
                w->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
            } else {
                w->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            }
        }
        scroll->horizontalScrollBar()->setValue(0);
        // the stack gives the focus to the first field of the new page: not wanted (a blue field)
        auto f = QApplication::focusWidget();
        if(f != nullptr && scroll_content->currentWidget() != nullptr &&
           scroll_content->currentWidget()->isAncestorOf(f)) {
            scroll->setFocus(Qt::OtherFocusReason);
        }
        fitContent();
    });

    search_bar = new QLineEdit(this);
    search_bar->setPlaceholderText("search");
    search_bar->setClearButtonEnabled(true);
    search_layout->addWidget(search_bar, 1);
    search_layout->addWidget(button_save);

    connect(path, &QStackedWidget::currentChanged, this, [=,this](int index) {
        path_row->setVisible(index != root_path_index);
    });

    auto search_callback = [=,this](const QString str) {
        if(str == "") {
            restore_searched_items();
            scroll_content->setCurrentIndex(last_widget_index);
            path->setCurrentIndex(last_path_index);
        } else {
            populate_search_results(str);
            scroll_content->setCurrentIndex(search_result_index);
            path->setCurrentIndex(search_path_index);
        }
    };

    connect(
        search_bar, &QLineEdit::textChanged,
        search_callback
    );

    connect(
        search_bar, &QLineEdit::returnPressed,
        [=,this]() {
            auto str = search_bar->text();
            search_callback(str);
    }
    );

    //setStyleSheet("QWidget{background-color: #31363b;} QLabel{color:white;} QAbstractButton{color:white;} QLineEdit{color:white;}");
    //setAutoFillBackground(true);

    connect(DispatcherUi::get(), &DispatcherUi::settingUpdated, this, &SettingsViewer::updateSettings);

    init(ac_id);
}

/// The list as tall as the page shown (rows open and close): never squeezed, no space below.
void SettingsViewer::fitContent() {
    QTimer::singleShot(0, this, [this]() {
        auto page = scroll_content->currentWidget();
        if(page == nullptr) {
            return;
        }
        if(page->layout()) {
            page->layout()->invalidate();
            page->layout()->activate();
        }
        int h = page->layout() ? page->layout()->sizeHint().height() : page->sizeHint().height();
        scroll_content->setMinimumHeight(h);
        scroll_content->setMaximumHeight(QWIDGETSIZE_MAX);
        scroll_content->updateGeometry();
    });
}

bool SettingsViewer::eventFilter(QObject *object, QEvent *event)
{
    if(event->type() == QEvent::FocusIn && field_focus.count(object)) {
        field_focus[object](static_cast<QFocusEvent*>(event)->reason());
        return false;
    }
    if(event->type() == QEvent::MouseButtonPress && field_focus.count(object)) {
        field_focus[object](Qt::MouseFocusReason);     // a tap in a field that has the focus
        return false;
    }
    if(event->type() == QEvent::MouseButtonDblClick && name_labels.contains(object)) {
        // every row open: ask the aircraft its value (as the old GCS); folded: two clicks open
        // and close again, nothing more
        if(rows_open && ask_buttons.count(name_labels[object])) {
            ask_buttons[name_labels[object]]->click();
        }
        return true;
    }
    if(event->type() == QEvent::MouseButtonRelease && name_labels.contains(object)) {
        auto setting = name_labels[object];
        if(toggle_slider.count(setting)) {
            toggle_slider[setting]();
        }
        return true;
    }
    if(search_bar->hasFocus() && event->type() == QEvent::KeyPress) {
        return true;    //interrupt event
    }
    return false;
}

void SettingsViewer::init(QString ac_id) {
    rows_open = getAppSettings().value("settings/rows_open", false).toBool();
    this->ac_id = ac_id;
    auto settings = AircraftManager::get()->getAircraft(ac_id)->getSettingMenu();
    create_widgets(settings, QList<SettingMenu*>());
    last_widget_index = widgets_indexes[settings];
    last_path_index = path_indexes[settings];

    auto result_widget = new QWidget(scroll_content);
    auto result_layout = new QVBoxLayout(result_widget);
    result_layout->addStretch();
    result_widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);   // until shown
    search_result_index = scroll_content->addWidget(result_widget);
    auto search_label = new QLabel("  searching...", path);
    search_path_index = path->addWidget(search_label);


    scroll_content->setCurrentIndex(widgets_indexes[settings]);

    connect(
        button_home, &QToolButton::clicked, this,
        [=,this]() {
            scroll_content->setCurrentIndex(widgets_indexes[settings]);
            path->setCurrentIndex(path_indexes[settings]);
            scroll->verticalScrollBar()->setValue(0);
            search_bar->clear();
        }
    );

    connect(
        button_save, &QToolButton::clicked, this,
        [=,this]() {
            auto saver_dialog = new SettingSaver(ac_id, this);
            saver_dialog->showMovable();
        }
    );



}

void SettingsViewer::create_widgets(SettingMenu* setting_menu, QList<SettingMenu*> stack) {
    auto widget = new QWidget(scroll_content);
    auto path_widget = new QWidget(this);
    auto menu_layout = new QVBoxLayout(widget);
    auto current_path_layout = new QHBoxLayout(path_widget);
    current_path_layout->setContentsMargins(0, 0, 0, 0);

    auto new_stack = QList<SettingMenu*>(stack);
    new_stack.append(setting_menu);

    if(new_stack.size()>0) {
        for(auto setmm = std::next(new_stack.begin()); setmm != new_stack.end(); ++setmm) {
            auto setm = (*setmm);
            auto sep = new QLabel(QString::fromUtf8("\xE2\x80\xBA"), path_widget);
            sep->setObjectName("setting_path_sep");
            auto button = new QPushButton(setm->getName(), path_widget);
            button->setObjectName("setting_path");
            sep->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
            button->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
            current_path_layout->addWidget(sep);
            current_path_layout->addWidget(button);

            connect(
                button, &QPushButton::clicked, this,
                [=,this]() {

                    scroll_content->setCurrentIndex(widgets_indexes[setm]);
                    path->setCurrentIndex(path_indexes[setm]);
                    restore_searched_items();
                    last_widget_index = widgets_indexes[setm];
                    last_path_index = path_indexes[setm];
                    scroll->verticalScrollBar()->setValue(0);
                }
            );
        }
        current_path_layout->addStretch();
    }

    int index_path = path->addWidget(path_widget);
    path_indexes[setting_menu] = index_path;
    if(stack.isEmpty()) {
        root_path_index = index_path;
        path_row->setVisible(path->currentIndex() != root_path_index);
    }

    // the groups in one block, the settings below it without space between them
    menu_layout->setSpacing(0);
    menu_layout->setContentsMargins(0, 4, 0, 4);
    menu_layout->setSpacing(8);     // room between two settings
    QFrame* groups_box = nullptr;
    QVBoxLayout* groups_lay = nullptr;
    QPushButton* last_group = nullptr;
    for(auto sets: setting_menu->getSettingMenus()) {
        if(groups_box == nullptr) {
            groups_box = new QFrame(widget);
            groups_box->setObjectName("setting_groups");
            groups_lay = new QVBoxLayout(groups_box);
            groups_lay->setContentsMargins(1, 1, 1, 1);
            groups_lay->setSpacing(0);
            menu_layout->addWidget(groups_box);
            menu_layout->addSpacing(8);
        }
        auto button = new QPushButton();
        button->setObjectName("setting_group");
        auto button_lay = new QHBoxLayout(button);
        button_lay->setContentsMargins(8, 0, 8, 0);
        // a folder, the name, and what is in it: it opens more settings
        auto icon_label = new QLabel(button);
        icon_label->setPixmap(QIcon(user_or_app_path("pictures/setting_group.svg")).pixmap(18, 18));
        button_lay->addWidget(icon_label);
        auto name_label = new QLabel(sets->getName(), button);
        name_label->setObjectName("setting_group_name");
        int nb_settings = sets->getSettings().size();
        int nb_groups = sets->getSettingMenus().size();
        QStringList what;
        if(nb_groups > 0) {
            what.append(QString("%1 %2").arg(nb_groups).arg(nb_groups == 1 ? "group" : "groups"));
        }
        if(nb_settings > 0) {
            what.append(QString("%1 %2").arg(nb_settings).arg(nb_settings == 1 ? "setting" : "settings"));
        }
        auto count_label = new QLabel(what.join(", ") + "  \u203A", button);
        count_label->setObjectName("setting_unit");
        button->setToolTip("Open " + sets->getName());
        button->setCursor(Qt::PointingHandCursor);
        for(auto l: {icon_label, name_label, count_label}) {
            l->setAttribute(Qt::WA_TransparentForMouseEvents);
            l->setStyleSheet("background: transparent; border: 0;");    // the row's background
        }
        button_lay->addWidget(name_label, 1);
        button_lay->addWidget(count_label);
        button->setMinimumHeight(40);     // touch
        groups_lay->addWidget(button);
        last_group = button;

        setting_menu_widgets[sets] = button;

        create_widgets(sets, new_stack);
        connect(
            button, &QPushButton::clicked, this,
            [=,this]() {
                scroll_content->setCurrentIndex(widgets_indexes[sets]);
                path->setCurrentIndex(path_indexes[sets]);
                restore_searched_items();
                last_widget_index = widgets_indexes[sets];
                last_path_index = path_indexes[sets];
                scroll->verticalScrollBar()->setValue(0);
            }
        );
    }
    if(last_group != nullptr) {
        last_group->setProperty("last", true);     // no line below the last one
    }
    for(auto &set: setting_menu->getSettings()) {
        auto setting_widget = makeSettingWidget(set, widget);
        menu_layout->addWidget(setting_widget);
        setting_widgets[set] = setting_widget;
    }
    menu_layout->addStretch();
    widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);   // until shown
    int index_widget = scroll_content->addWidget(widget);
    widgets_indexes[setting_menu] = index_widget;
}

void SettingsViewer::restore_searched_items() {
    while (pos_hist.size() > 0) {
        auto pos = pos_hist.takeLast();
        pos.layout->insertWidget(pos.index, pos.widget);
        if(auto group = pos.widget->findChild<QLabel*>("setting_group_label")) {
            group->hide();
        }
    }
}

void SettingsViewer::populate_search_results(QString searched) {
    restore_searched_items();

    QBoxLayout* l = dynamic_cast<QBoxLayout*>(scroll_content->widget(search_result_index)->layout());

    // search in Menus
    for(auto sets: setting_menu_widgets.keys()) {
        QString name = sets->getName();
        if(name.toLower().contains(searched.toLower())) {
            auto w = setting_menu_widgets[sets];
            int index = w->parentWidget()->layout()->indexOf(w);
            pos_hist.push_back(PositionHistory{w, dynamic_cast<QBoxLayout*>(w->parentWidget()->layout()), index});
            l->insertWidget(l->count()-1, w);
        }
    }

    //search in settings
    for(auto set: setting_widgets.keys()) {
        QString name = set->getName();
        if(name.toLower().contains(searched.toLower())) {
            auto w = setting_widgets[set];
            int index = w->parentWidget()->layout()->indexOf(w);
            pos_hist.push_back(PositionHistory{w, dynamic_cast<QBoxLayout*>(w->parentWidget()->layout()), index});
            l->insertWidget(l->count()-1, w);
            if(auto group = w->findChild<QLabel*>("setting_group_label")) {
                group->setVisible(!group->text().isEmpty());
            }
        }
    }


}



/**
 * One setting on one line: its name and its control (a few choices as buttons, more as a list,
 * on/off as a switch, a number as a field with a slider below while editing). The control shows
 * the value on the aircraft; changing it marks the row and shows Undo / Send. A sent value stays
 * marked until the aircraft confirms it. Open (click on the name), the row also has Previous and
 * Initial (sent at once), Ask (drops a change, asks the aircraft its value) and Send.
 */
QWidget* SettingsViewer::makeSettingWidget(Setting* setting, QWidget* parent) {
    QWidget* widget = new QWidget(parent);
    widget->setObjectName("setting_row");
    widget->setAttribute(Qt::WA_StyledBackground);
    rows[setting] = widget;
    QVBoxLayout* vLay = new QVBoxLayout(widget);
    vLay->setContentsMargins(8, 2, 2, 2);     // after the bar at the left
    vLay->setSpacing(2);
    widget->setMinimumHeight(38);     // touch
    QHBoxLayout* hlay = new QHBoxLayout();
    hlay->setSpacing(6);
    vLay->addLayout(hlay);

    QLabel* label = new QLabel(setting->getName(), widget);
    label->setObjectName("setting_name");
    label->setMinimumWidth(30);
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    label->setToolTip(rows_open ? setting->getName() + " (double click: ask the aircraft its value)" :
                      setting->getName() + " (click: open, with the slider and the buttons)");
    label->installEventFilter(this);
    name_labels[label] = setting;
    hlay->addWidget(label, 1);

    // the group, shown in search results
    QLabel* group_label = new QLabel(widget);
    group_label->setObjectName("setting_group_label");
    if(auto menu = qobject_cast<SettingMenu*>(setting->parent())) {
        group_label->setText(menu->getName());
    }
    group_label->hide();
    hlay->addWidget(group_label);

    // the value on the aircraft, while the control shows another one
    auto aircraft_label = new QLabel(widget);
    aircraft_label->setObjectName("setting_aircraft");
    aircraft_label->hide();
    hlay->addWidget(aircraft_label);
    aircraft_labels[setting] = aircraft_label;

    QHBoxLayout* control_lay = new QHBoxLayout();
    control_lay->setSpacing(4);
    hlay->addLayout(control_lay);

    QString unit = setting->getDisplayUnit();
    QLabel* unit_label = nullptr;

    // second line (numbers): the slider; last line: Undo / Send while changed
    QHBoxLayout* slider_lay = new QHBoxLayout();
    vLay->addLayout(slider_lay);
    auto actions = new QWidget(widget);
    actions->setObjectName("setting_actions");
    auto actions_lay = new QHBoxLayout(actions);
    actions_lay->setContentsMargins(0, 0, 0, 2);
    // the previous and the initial value, sent at once
    auto prev_btn = new QPushButton(QString::fromUtf8("\u21A9 Previous"), actions);
    prev_btn->setObjectName("setting_undo");
    auto init_btn = new QPushButton(QString::fromUtf8("\u21BA Initial"), actions);
    init_btn->setObjectName("setting_undo");
    auto ask_btn = new QPushButton(QString::fromUtf8("\u21BB Ask"), actions);
    ask_btn->setObjectName("setting_undo");
    ask_buttons[setting] = ask_btn;
    ask_btn->setToolTip("Request current value");
    actions_lay->addWidget(prev_btn);
    actions_lay->addWidget(init_btn);
    actions_lay->addStretch(1);
    auto send_btn = new QPushButton("Send", actions);
    send_btn->setObjectName("setting_send");
    actions_lay->addWidget(ask_btn);
    actions_lay->addWidget(send_btn);
    actions->hide();
    vLay->addWidget(actions);
    // a number outside the range: said in red, and Send asks first
    auto range_note = new QLabel(widget);
    range_note->setObjectName("setting_out_of_range");
    range_note->hide();
    vLay->insertWidget(vLay->indexOf(actions), range_note);

    initialized[setting] = false;
    dirty[setting] = false;
    std::function<float()> control_value;           // in the displayed unit
    std::function<void(bool)> show_editor = [](bool) {};
    // opened by a click on the name: the slider (numbers) and the buttons
    auto expanded = std::make_shared<bool>(rows_open);     // GCS setting: all rows open
    auto opened_by_field = std::make_shared<bool>(false);
    auto update_actions = [=,this]() {
        actions->setVisible(dirty[setting] || *expanded);
        if(widget->property("open").toBool() != *expanded) {     // the bar at the left in blue
            widget->setProperty("open", *expanded);
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
        }
        // the previous value: the one before the last send from here (once there was a send)
        auto previous = setting->getPreviousValue();
        prev_btn->setEnabled(previous.has_value());
        prev_btn->setToolTip(previous.has_value() ?
            QString("Previous: %1").arg(previous.value() * setting->getAltUnitCoef()) :
            QString("No previous value"));
        // the initial value: the first value received from the aircraft (once it is known)
        auto initial = setting->getInitialValue();
        init_btn->setEnabled(initial.has_value());
        init_btn->setToolTip(initial.has_value() ?
            QString("Initial: %1").arg(initial.value() * setting->getAltUnitCoef()) :
            QString("Initial value unknown"));
    };
    refresh_actions[setting] = update_actions;

    auto [min, max, step] = setting->getBounds();
    auto values = setting->getValues();
    bool updating = false;
    auto changed = std::make_shared<std::function<void()>>([](){});

    if(values.size() > 2) {
        // named values: buttons for a few short ones, else a list
        bool short_names = values.size() <= 3;
        for(auto &v: values) {
            short_names = short_names && v.size() <= 8;
        }
        if(short_names) {
            auto group = new QButtonGroup(widget);
            group->setExclusive(true);
            for(int i=0; i<values.size(); i++) {
                auto b = new QPushButton(values[i], widget);
                b->setObjectName("setting_choice");
                b->setCheckable(true);
                group->addButton(b, i);
                control_lay->addWidget(b);
            }
            setters[setting] = [group](double value) {
                if(auto b = group->button(static_cast<int>(value))) {
                    b->setChecked(true);
                }
            };
            control_value = [group]() {return static_cast<float>(group->checkedId());};
            connect(group, &QButtonGroup::idClicked, this, [=](int) {(*changed)();});
        } else {
            QComboBox* combo = new QComboBox(widget);
            combo->setObjectName("setting_combo");
            for(auto &v: values) {
                combo->addItem(v);
            }
            combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
            control_lay->addWidget(combo);
            setters[setting] = [combo](double value) {
                QSignalBlocker b(combo);
                combo->setCurrentIndex(static_cast<int>(value));
            };
            control_value = [combo]() {return static_cast<float>(combo->currentIndex());};
            connect(combo, qOverload<int>(&QComboBox::activated), this, [=](int) {(*changed)();});
        }
        label_setters[setting] = [=](double value) {
            int i = static_cast<int>(value);
            aircraft_label->setText(i >= 0 && i < values.size() ? values[i] : QString::number(value));
        };

    } else if(values.size() == 2 || abs(min+step-max) < 0.0001) {
        QString s1 = values.size() == 2 ? values[0] : QString::number(min);
        QString s2 = values.size() == 2 ? values[1] : QString::number(max);
        auto state = new QLabel(s1, widget);
        state->setObjectName("setting_unit");
        Switch* sw = new Switch();
        control_lay->addWidget(state);
        control_lay->addWidget(sw);
        setters[setting] = [=, min=min, step=step](double value) {
            QSignalBlocker b(sw);
            sw->setChecked(value > (min + step/2));
            state->setText(sw->isChecked() ? s2 : s1);
        };
        control_value = [sw, min=min, max=max]() {return sw->isChecked() ? max : min;};
        connect(sw, &QAbstractButton::toggled, this, [=](bool on) {
            state->setText(on ? s2 : s1);
            (*changed)();
        });
        label_setters[setting] = [=, min=min, step=step](double value) {
            aircraft_label->setText(value < (min + step/2) ? s1 : s2);
        };

    } else if(abs(max-min) < 0.00001) {
        // a single value: a button that sends it
        float value = min;
        auto b = new QPushButton(QString("Send %1").arg(value), widget);
        b->setObjectName("setting_choice");
        control_lay->addWidget(b);
        setters[setting] = [](double) {};
        control_value = [value]() {return value;};
        label_setters[setting] = [=](double v) {aircraft_label->setText(QString::number(v));};
        connect(b, &QPushButton::clicked, this, [=,this]() {
            send(setting, value / setting->getSendCoef());
        });

    } else {
        // a number: a field (Enter sends), the slider below while editing
        int precision = step < 1 ? static_cast<int>(ceil(abs(log10(step)))) : 0;
        auto edit = new QLineEdit(widget);
        edit->setObjectName("setting_number");
        edit->setAlignment(Qt::AlignRight);
        edit->setFixedWidth(80);
        edit->setToolTip(QString("%1 to %2%3 (Enter: send; other values are possible)")
                         .arg(min).arg(max).arg(unit.isEmpty() ? QString() : " " + unit));
        control_lay->addWidget(edit);
        DoubleSlider* slider = new DoubleSlider(Qt::Horizontal, widget);
        slider->setDoubleRange(min, max, step);
        auto lmin = new QLabel(QString::number(min), widget);
        auto lmax = new QLabel(QString::number(max), widget);
        lmin->setObjectName("setting_unit");
        lmax->setObjectName("setting_unit");
        slider_lay->addWidget(lmin);
        slider_lay->addWidget(slider, 1);
        slider_lay->addWidget(lmax);
        show_editor = [=,this](bool show) {
            bool visible = show || *expanded;
            for(auto w: std::initializer_list<QWidget*>{lmin, slider, lmax}) {
                w->setVisible(visible);
            }
        };
        show_editor(false);
        setters[setting] = [=](double value) {
            QSignalBlocker b1(slider);
            QSignalBlocker b2(edit);
            slider->setDoubleValue(value);
            edit->setText(QString::number(value, 'f', precision));
        };
        control_value = [=]() {
            bool ok;
            double v = edit->text().toDouble(&ok);
            return static_cast<float>(ok ? v : slider->doubleValue());
        };
        label_setters[setting] = [=](double value) {
            aircraft_label->setText(QString::number(value, 'f', precision));
        };
        connect(slider, &DoubleSlider::doubleValueChanged, this, [=](double value) {
            QSignalBlocker b(edit);
            edit->setText(QString::number(value, 'f', precision));
            (*changed)();
        });
        connect(edit, &QLineEdit::textEdited, this, [=,this](QString txt) {
            bool ok;
            double v = txt.toDouble(&ok);
            bool out = ok && (v < min || v > max);
            edit->setProperty("out_of_range", out);
            edit->style()->unpolish(edit);
            edit->style()->polish(edit);
            range_note->setText(QString("Outside the range %1 \u2026 %2%3").arg(min).arg(max)
                                .arg(unit.isEmpty() ? QString() : " " + unit));
            if(range_note->isVisible() != out) {
                range_note->setVisible(out);
                fitContent();
            }
            if(ok) {
                QSignalBlocker b(slider);
                slider->setDoubleValue(qBound(static_cast<double>(min), v, static_cast<double>(qMax(min, max))));
            }
            (*changed)();
        });
        connect(edit, &QLineEdit::returnPressed, send_btn, &QPushButton::click);
        // using the field opens the whole row (slider and buttons), as a click on the name; it
        // closes again when leaving it without a change
        // only for a tap, Tab or a shortcut: opening a group gives the focus to its first field
        field_focus[edit] = [=,this](Qt::FocusReason reason) {
            if(!*expanded && (reason == Qt::MouseFocusReason || reason == Qt::TabFocusReason ||
                              reason == Qt::BacktabFocusReason || reason == Qt::ShortcutFocusReason)) {
                *expanded = true;
                *opened_by_field = true;
                refresh_actions[setting]();
                show_editor(false);
                fitContent();
            }
        };
        edit->installEventFilter(this);
        connect(qApp, &QApplication::focusChanged, edit, [=,this](QWidget*, QWidget* now) {
            if(now != edit && *opened_by_field && !dirty[setting] &&
                      !(now != nullptr && widget->isAncestorOf(now))) {
                *expanded = false;
                *opened_by_field = false;
                refresh_actions[setting]();
                show_editor(false);
                fitContent();
            }
        });
    }
    if(!unit.isEmpty() && (values.size() == 0 && abs(max-min) >= 0.00001 && abs(min+step-max) >= 0.0001)) {
        unit_label = new QLabel(unit, widget);
        unit_label->setObjectName("setting_unit");
        control_lay->addWidget(unit_label);
    }

    // a click on the name opens the row: the slider and the buttons
    if(!rows_open) {
        label->setCursor(Qt::PointingHandCursor);
    }
    toggle_slider[setting] = [=,this]() {
        if(rows_open) {
            return;     // every row open: nothing to fold
        }
        *expanded = !*expanded;
        *opened_by_field = false;
        show_editor(false);
        update_actions();
        fitContent();
    };
    update_actions();

    // changed here: mark the row, show the aircraft's value and Ask / Send
    set_dirty[setting] = [=,this](bool d) {
        dirty[setting] = d;
        update_actions();
        fitContent();
        if(!d) {
            show_editor(false);
        }
        widget->setProperty("editing", d);
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        showAircraftValue(setting);
    };
    *changed = [=,this]() {
        if(!updating) {
            set_dirty[setting](true);
        }
    };

    connect(send_btn, &QPushButton::clicked, this, [=,this, min=min, max=max]() {
        float v = control_value();
        if(v < min - 1e-6 || v > max + 1e-6) {
            QString u = unit.isEmpty() ? QString() : " " + unit;
            if(!CardDialog::ask(this, QString("Send %1%2 to %3?").arg(v).arg(u, setting->getName()),
                                QString("This is outside its range (%1 to %2%3).").arg(min).arg(max).arg(u),
                                "Send anyway", CardDialog::DANGER)) {
                return;
            }
        }
        send(setting, v / setting->getSendCoef());
        range_note->hide();
        set_dirty[setting](false);
    });
    // Ask: a change here is dropped (back to the last known value), the control shows the answer
    connect(ask_btn, &QPushButton::clicked, this, [=,this]() {
        range_note->hide();
        if(aircraft_values.count(setting)) {
            setters[setting](aircraft_values[setting]);
        }
        set_dirty[setting](false);
        requestValue(setting);
    });
    // Previous and Initial: the control shows the value sent (amber until it is confirmed)
    auto send_shown = [=,this](float value) {
        set_dirty[setting](false);
        setters[setting](value * setting->getAltUnitCoef());
        send(setting, value);
    };
    connect(prev_btn, &QPushButton::clicked, this, [=,this]() {
        auto previous = setting->getPreviousValue();
        if(previous.has_value()) {
            send_shown(previous.value());
        }
    });
    connect(init_btn, &QPushButton::clicked, this, [=,this]() {
        auto initial_value = setting->getInitialValue();
        if(initial_value.has_value()) {
            send_shown(initial_value.value());
        }
    });

    return widget;
}

/// The value on the aircraft next to the control, when the control shows another value
/// (changed here, or sent and not yet confirmed); amber while a sent value is not confirmed.
void SettingsViewer::showAircraftValue(Setting* setting) {
    if(!aircraft_labels.count(setting)) {
        return;
    }
    auto l = aircraft_labels[setting];
    bool is_pending = pending.count(setting) > 0;
    bool known = aircraft_values.count(setting) > 0;
    bool state = !l->property("state").toString().isEmpty();
    l->setVisible(((dirty[setting] || is_pending) && known) || state);
    l->setToolTip(is_pending ?
        QString("On the aircraft; sent %1, not yet confirmed").arg(pending[setting] * setting->getAltUnitCoef()) :
        QString("Value on the aircraft"));
    auto row = rows[setting];
    if(row->property("pending").toBool() != is_pending) {
        row->setProperty("pending", is_pending);
        row->style()->unpolish(row);
        row->style()->polish(row);
    }
}

void SettingsViewer::requestValue(Setting* setting, int attempt) {
    if(aircraft_labels.count(setting)) {
        aircraft_labels[setting]->setText(attempt > 1 ? QString("asking\u2026 %1/3").arg(attempt) : QString("asking\u2026"));
        aircraft_labels[setting]->setToolTip("Asked the aircraft its value");
        setLabelState(aircraft_labels[setting], "asking");
        aircraft_labels[setting]->show();
    }
    asking[setting] = attempt;
    pprzlink::Message getSetting(PprzDispatcher::get()->getDict()->getDefinition("GET_DL_SETTING"));
    getSetting.addField("ac_id", ac_id);
    getSetting.addField("index", setting->getNo());
    PprzDispatcher::get()->sendMessage(getSetting);
    // no answer: again, 3 times
    QTimer::singleShot(3000, this, [=,this]() {
        if(asking.count(setting) && asking[setting] == attempt) {
            if(attempt < 3) {
                requestValue(setting, attempt + 1);
            } else {
                asking.erase(setting);
                if(aircraft_labels.count(setting)) {
                    aircraft_labels[setting]->setText("no answer");
                    aircraft_labels[setting]->setToolTip("The aircraft did not answer (3 times); \u21BB Ask to ask again");
                    setLabelState(aircraft_labels[setting], "failed");
                }
            }
        }
    });
}

/// Send a value (in the aircraft's unit); it shows as not confirmed until the aircraft reports it.
void SettingsViewer::send(Setting* setting, float value) {
    AircraftManager::get()->getAircraft(ac_id)->setSetting(setting, value);
    setting->setUserValue(value);
    pending[setting] = value;
    markPending(setting, true);
    if(refresh_actions.count(setting)) {
        refresh_actions[setting]();     // the previous value changed
    }
}

void SettingsViewer::markPending(Setting* setting, bool is_pending) {
    (void)is_pending;
    showAircraftValue(setting);
}

void SettingsViewer::updateSettings(QString id, Setting* setting, float value) {
    if(ac_id == id && label_setters.count(setting)) {
        auto val = setting->getAltUnitCoef() * value;
        aircraft_values[setting] = val;
        bool was_asked = asking.count(setting) > 0 ||
            (aircraft_labels.count(setting) && aircraft_labels[setting]->property("state").toString() == "failed");
        asking.erase(setting);     // answered
        label_setters[setting](val);
        setting->setValue(value);
        if(pending.count(setting) && abs(pending[setting] - value) < 1e-4 * std::max(1.f, abs(value))) {
            pending.erase(setting);
        }
        // the control follows the aircraft, unless it is being changed here or shows a sent value,
        // or is being used (e.g. the cursor in the field: it would jump)
        auto focus = QApplication::focusWidget();
        // only typing in the field (or holding the slider) blocks it, not e.g. the Ask button
        bool in_use = focus != nullptr && rows.count(setting) && rows[setting]->isAncestorOf(focus) &&
                      (qobject_cast<QLineEdit*>(focus) != nullptr ||
                       (qobject_cast<QAbstractSlider*>(focus) != nullptr &&
                        qobject_cast<QAbstractSlider*>(focus)->isSliderDown()));
        if(!dirty[setting] && !pending.count(setting) && !in_use) {
            setters[setting](val);
        }
        if(!initialized[setting]) {
            setting->setInitialValue(value);
            initialized[setting] = true;
            if(refresh_actions.count(setting)) {
                refresh_actions[setting]();    // Initial can be used now
            }
        }
        markPending(setting, pending.count(setting) > 0);
        if(was_asked && aircraft_labels.count(setting)) {
            // the answer, in green for 2 s
            auto l = aircraft_labels[setting];
            setLabelState(l, "answered");
            l->setText(QString::fromUtf8("\u2713 ") + l->text());
            l->show();
            QTimer::singleShot(2000, l, [=,this]() {
                if(l->property("state").toString() == "answered") {
                    setLabelState(l, "");
                    label_setters[setting](aircraft_values[setting]);
                    showAircraftValue(setting);
                }
            });
        } else if(aircraft_labels.count(setting)) {
            setLabelState(aircraft_labels[setting], "");
        }
    }
}

void SettingsViewer::setLabelState(QLabel* l, QString state) {
    if(l->property("state").toString() != state) {
        l->setProperty("state", state);
        l->style()->unpolish(l);
        l->style()->polish(l);
    }
}



/**
 * Save the settings that changed into the airframe file: the changed ones (checked), the others
 * folded away, and the settings that are not in the airframe file in a note.
 */
SettingSaver::SettingSaver(QString ac_id, QWidget *parent) :
    CardDialog("Save settings to airframe", AircraftManager::get()->getAircraft(ac_id)->name(), parent,
               AircraftManager::get()->getAircraft(ac_id)->getColor()),
    ac_id(ac_id)
{
    setAttribute(Qt::WA_DeleteOnClose);
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    auto settings = ac->getSettingMenu()->getAllSettings();
    auto airframe = ac->getAirframe();
    auto params = airframe->getParams();

    struct Row {
        QString name;
        QString airframe_value;
        QString value;
        bool changed;
    };
    QList<Row> rows;
    QStringList unsavable;

    for(auto setting: settings) {
        auto param_name = setting->getParam();
        if(param_name == "" || !setting->getValue().has_value()) {
            continue;
        }
        bool found = false;
        for(auto &p: params) {
            if(p.name == param_name) {
                auto val = setting->getValue().value() * setting->getAltUnitCoef(p.unit);
                bool changed = abs(val - p.value.toFloat()) > 0.0001;
                rows.append({param_name, p.value, QString::number(val), changed});
                found = true;
                break;
            }
        }
        // not in the airframe file (a module default, or an included file): only worth saying when
        // it was changed since the start (nothing would be lost otherwise)
        auto initial = setting->getInitialValue();
        if(!found && initial.has_value() && abs(setting->getValue().value() - initial.value()) > 1e-4) {
            unsavable.append(param_name);
        }
    }
    std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {return a.changed && !b.changed;});
    int nb_changed = static_cast<int>(std::count_if(rows.begin(), rows.end(), [](const Row& r) {return r.changed;}));

    addText(nb_changed == 0 ? QString("No setting changed since the airframe was loaded") :
            nb_changed == 1 ? QString("1 setting changed since the airframe was loaded") :
                              QString("%1 settings changed since the airframe was loaded").arg(nb_changed));

    // the settings: changed ones first and checked, the others behind "Show N unchanged"
    auto grid_widget = new QWidget(this);
    auto grid = new QGridLayout(grid_widget);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(4);
    grid->setColumnStretch(1, 1);
    int r = 0;
    auto head = [&](QString t, int col) {
        auto l = new QLabel(t, grid_widget);
        l->setObjectName("card_muted");
        l->setStyleSheet("font-size: 11px; font-weight: bold;");
        grid->addWidget(l, r, col, col >= 2 ? Qt::AlignRight : Qt::AlignLeft);
    };
    head("Setting", 1);
    head("Airframe", 2);
    head("Now", 3);
    r++;

    QList<QCheckBox*> checks;
    QList<QWidget*> unchanged_widgets;
    for(auto &row: rows) {
        auto ck = new QCheckBox(grid_widget);
        ck->setChecked(row.changed);
        ck->setProperty("param", row.name);
        ck->setProperty("value", row.value);
        checks.append(ck);
        auto name = new QLabel(row.name, grid_widget);
        auto af = new QLabel(row.airframe_value, grid_widget);
        af->setObjectName("card_muted");
        auto now = new QLabel(row.value, grid_widget);
        if(row.changed) {
            now->setStyleSheet("font-weight: bold; color: #1d5fc9;");
        } else {
            now->setObjectName("card_muted");
            unchanged_widgets << ck << name << af << now;
        }
        for(auto l: {af, now}) {
            l->setStyleSheet(l->styleSheet() + "font-family: Monospace;");
        }
        grid->addWidget(ck, r, 0);
        grid->addWidget(name, r, 1);
        grid->addWidget(af, r, 2, Qt::AlignRight);
        grid->addWidget(now, r, 3, Qt::AlignRight);
        r++;
    }

    auto scroll = new QScrollArea(this);
    scroll->setWidget(grid_widget);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // as wide as the longest name with both values (measured with every row shown: no column
    // cut off behind the scroll bar when the unchanged ones are shown)
    int content_w = grid_widget->sizeHint().width() + scroll->verticalScrollBar()->sizeHint().width() + 8;
    scroll->setMinimumWidth(qBound(380, content_w, parent != nullptr ? qMax(380, parent->width() - 60) : 900));
    body()->addWidget(scroll);
    auto fit = [=,this]() {
        grid->invalidate();
        grid->activate();
        scroll->setFixedHeight(qMin(grid->sizeHint().height() + 2, 360));
        layout()->invalidate();
        layout()->activate();
        resize(width(), layout()->sizeHint().height());
    };

    int nb_unchanged = rows.size() - nb_changed;
    if(nb_unchanged > 0) {
        auto more = new QPushButton(QString("▸ Show %1 unchanged").arg(nb_unchanged), this);
        more->setFlat(true);
        more->setCheckable(true);
        more->setObjectName("card_muted");
        more->setStyleSheet("text-align: left; border: 0; padding: 0; background: transparent;");
        more->setCursor(Qt::PointingHandCursor);
        body()->addWidget(more);
        for(auto w: unchanged_widgets) {
            w->setVisible(false);
        }
        connect(more, &QPushButton::toggled, this, [=,this](bool show) {
            more->setText(QString("%1 %2 %3 unchanged").arg(show ? "▾" : "▸", show ? "Hide" : "Show").arg(nb_unchanged));
            for(auto w: unchanged_widgets) {
                w->setVisible(show);
            }
            QTimer::singleShot(0, this, fit);
        });
    }

    if(!unsavable.isEmpty()) {
        // the names folded when there are many
        QString head = QString("%1 not in the airframe file (a module default or an included file), "
                               "so not saved:")
                .arg(unsavable.size() == 1 ? "1 changed setting is" : QString("%1 changed settings are").arg(unsavable.size()));
        auto note = addText(head + " " + unsavable.join(", "), "card_note");
        if(unsavable.size() > 5) {
            note->setText(head + " " + unsavable.mid(0, 5).join(", ") + QString::fromUtf8(", \u2026"));
            note->setToolTip(unsavable.join("\n"));
        }
    }

    // the airframe file, when it is a local file
    auto uri = ac->getConfig()->getAirframeUri();
    QString path;
    if(uri.startsWith("file://")) {
        path = uri.mid(QString("file://").size());
    }
    QCheckBox* keep_copy = nullptr;
    if(!path.isEmpty()) {
        keep_copy = new QCheckBox("Keep a copy of the old file", this);
        keep_copy->setChecked(true);
        body()->addWidget(keep_copy);
        auto path_label = addText(path);
        path_label->setStyleSheet("font-family: Monospace; font-size: 11px;");
        path_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }

    auto cancel = addButton("Cancel");
    auto save_as = addButton("Save as…");
    auto save = addButton("Save", PRIMARY);
    save->setVisible(!path.isEmpty());
    if(path.isEmpty()) {
        save_as->setObjectName("card_button_primary");
        save_as->setDefault(true);
    }

    auto changed_params = [=]() {
        QMap<QString, QString> changed;
        for(auto ck: checks) {
            if(ck->isChecked()) {
                changed[ck->property("param").toString()] = ck->property("value").toString();
            }
        }
        return changed;
    };
    auto update_buttons = [=]() {
        bool any = !changed_params().isEmpty();
        save->setEnabled(any);
        save_as->setEnabled(any);
    };
    for(auto ck: checks) {
        connect(ck, &QCheckBox::toggled, this, update_buttons);
    }
    update_buttons();

    auto write = [=,this](QString file) {
        if(file == path && keep_copy != nullptr && keep_copy->isChecked()) {
            // copy original file before overwriting it
            auto suffix = QDateTime::currentDateTime().toString("_yy_MM_dd__hh_mm_ss") + ".xml";
            QFile::copy(path, path.left(path.length() - 4) + suffix);
        }
        airframe->setParams(changed_params());
        airframe->saveSettings(file);
        accept();
    };

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(save, &QPushButton::clicked, this, [=]() {write(path);});
    connect(save_as, &QPushButton::clicked, this, [=,this]() {
        QString dir = path.isEmpty() ? appConfig()->value("PAPARAZZI_HOME").toString() + "/conf/airframes" : path;
        auto file = QFileDialog::getSaveFileName(this, "Save airframe as", dir, "XML files (*.xml)", 0, QFileDialog::DontUseNativeDialog);
        if(file != "") {
            write(file);
        }
    });

    fit();
}
