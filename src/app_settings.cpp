#include "card_dialog.h"
#include "app_settings.h"
#include "dispatcher_ui.h"
#include "map_notifications.h"
#include "switch.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include <QString>
#include <iostream>
#include <QDir>
#include <QVariant>
#include <QStandardPaths>
#include <QSettings>
#include <QtWidgets>
#include <QFileInfo>
#include <pprzmain.h>
#include "gcs_utils.h"
#include "globalconfig.h"

void default_setting(QString key, QVariant value) {
    auto settings = getAppSettings();

    if(!settings.contains(key)) {
        settings.setValue(key, value);
    }
}

void set_app_settings() {
    auto settings = getAppSettings();

    default_setting("ivy/name", "PprzGCS");
    default_setting("pprzlink/id", "pprzcontrol");

    if(!appConfig()->contains("IVY_BUS")) {
        appConfig()->setValue("IVY_BUS", "127.255.255.255:2010");
    }

    auto PAPARAZZI_HOME = QString(qgetenv("PAPARAZZI_HOME"));
    auto PAPARAZZI_SRC = QString(qgetenv("PAPARAZZI_SRC"));
    if(PAPARAZZI_HOME == "" || PAPARAZZI_SRC == "") {
        throw std::runtime_error("env PAPARAZZI_HOME and PAPARAZZI_SRC must be set!");
    }

    appConfig()->setValue("PAPARAZZI_HOME", PAPARAZZI_HOME);
    appConfig()->setValue("PAPARAZZI_SRC", PAPARAZZI_SRC);

    auto messages = appConfig()->value("PAPARAZZI_HOME").toString() + "/var/messages.xml";
    appConfig()->setValue("MESSAGES", messages);

    auto gcs_icons_path = appConfig()->value("PAPARAZZI_HOME").toString() + "/data/pictures/gcs_icons";
    appConfig()->setValue("GCS_ICONS_PATH", gcs_icons_path);

    default_setting("map/default_tiles", "Google");

    default_setting("map/move_hyteresis", 20);
    default_setting("map/waypoint/size", 8);
    default_setting("map/gcs_icon_size", 16);
    default_setting("map/gcs_icon_color", "#ffff00");
    default_setting("map/circle/stroke", 4);
    default_setting("map/size_highlight_factor", 1.5);

    default_setting("map/z_values/shapes", 50);
    default_setting("map/z_values/unhighlighted", 100);
    default_setting("map/z_values/highlighted", 200);
    default_setting("map/z_values/aircraft", 400);
    default_setting("map/z_values/carrot", 500);
    default_setting("map/z_values/nav_shape", 600);

    default_setting("map/items_font", 18);
    default_setting("map/aircraft/size", 40);
    default_setting("map/intruder_size", 40);

    default_setting("map/tiles_path", appConfig()->value("USER_DATA_PATH").toString() + "/map");

    default_setting("map/aircraft/track_seconds", 60);
    default_setting("map/aircraft_status_icons", false);
    default_setting("map/aircraft_info_height", false);
    default_setting("map/aircraft_info_climb", false);
    default_setting("map/aircraft_info_speed", false);
    default_setting("map/aircraft_info_gcs", false);
    // the waypoint under the mouse: its name in blue, with a ring around it (or not), or nothing
    // (was the switch map/waypoint_hover_mark)
    if(!settings.contains("map/waypoint_hover_highlight") && settings.contains("map/waypoint_hover_mark")) {
        settings.setValue("map/waypoint_hover_highlight",
                          settings.value("map/waypoint_hover_mark").toBool() ? "Name and ring" : "Off");
        settings.remove("map/waypoint_hover_mark");
    }
    default_setting("map/waypoint_hover_highlight", "Name and ring");
    default_setting("map/waypoint_hover_dim", false);
    default_setting("map/waypoint_altitude", "On hover");
    default_setting("map/waypoint_altitude_always", "Height");
    default_setting("settings/rows_open", false);
    default_setting("settings/request_values", true);
    default_setting("strips/status_icons", true);
    default_setting("window/menu_title_bar", false);
    default_setting("map/follow_button", true);
    default_setting("map/aircraft/track_width", 1);

    default_setting("aircraft_default_color", "red");

    for(auto &k: MapNotifications::kinds()) {
        default_setting("notify/" + k.key, k.on);
    }
    default_setting("notify/sound", true);
    default_setting("notify/speech", false);
    default_setting("APP_STYLE_FILE", "default_style.qss");
    default_setting("APP_LAYOUT_FILE", "default_layout.xml");

}



SettingsEditor::SettingsEditor(bool standalone, QWidget* parent): QDialog(parent)
{
    setWindowTitle("PprzGCS application settings");
    auto lay = new QVBoxLayout(this);

    auto tabWidget = new QTabWidget(this);
    lay->addWidget(tabWidget);

    auto settings = getAppSettings();

    // Tab Ivy
    auto w_ivy = new QWidget(tabWidget);
    auto l_ivy = new QGridLayout(w_ivy);
    int row = 0;
    auto cb = addSetting("Ivy name", "ivy/name", w_ivy, l_ivy, row, Type::STRING);
    callbacks.append(cb);
    cb = addSetting("Pprzlink ID", "pprzlink/id", w_ivy, l_ivy, row, Type::STRING);
    callbacks.append(cb);
    finishTab(l_ivy, row);
    tabWidget->addTab(w_ivy, "Ivy");

    // Tab Map
    auto w_map = new QWidget(tabWidget);
    auto l_map = new QGridLayout(w_map);
    row = 0;

    addSection("Tiles", w_map, l_map, row);
    QVariant tpnv = GlobalConfig::get()->value("tile_providers_names");
    if(tpnv.isValid()) {
        cb = addSetting("Default tiles", "map/default_tiles", w_map, l_map, row, Type::STRING_COMBO, tpnv.toStringList());
        callbacks.append(cb);
    } else {
        cb = addSetting("Default tiles", "map/default_tiles", w_map, l_map, row, Type::STRING);
        callbacks.append(cb);
    }
    cb = addSetting("Tiles folder", "map/tiles_path", w_map, l_map, row, Type::PATH_DIR);
    callbacks.append(cb);

    addSection("Aircraft", w_map, l_map, row);
    cb = addSetting("Aircraft size", "map/aircraft/size", w_map, l_map, row, Type::INT);
    callbacks.append(cb);
    cb = addSetting("Track length (s)", "map/aircraft/track_seconds", w_map, l_map, row, Type::INT);
    callbacks.append(cb);
    cb = addSetting("Track width", "map/aircraft/track_width", w_map, l_map, row, Type::INT);
    callbacks.append(cb);
    cb = addSetting("Follow button", "map/follow_button", w_map, l_map, row, Type::BOOL);
    callbacks.append(cb);
    addHelp("The button left of the wind and compass, bottom right of the map. Click: the map follows the selected "
            "aircraft; again: also heading up (as File \u2192 Track AC); again: off. Right click or long press: follow the ground station, or keep the aircraft and the "
            "ground station in view (with a ground GPS). Dragging the map stops it.", w_map, l_map, row);
    live_settings << "map/follow_button";
    cb = addSetting("Battery and link icons", "map/aircraft_status_icons", w_map, l_map, row, Type::BOOL);
    callbacks.append(cb);
    addHelp("Small battery and link status icons next to each aircraft on the map.", w_map, l_map, row);
    cb = addSetting("Height next to the aircraft", "map/aircraft_info_height", w_map, l_map, row, Type::BOOL);
    callbacks.append(cb);
    addHelp("As set for the waypoints (Waypoints tab): above the ground (AGL with SRTM data, else H "
            "above the flight plan ground altitude) or above sea level (MSL).", w_map, l_map, row);
    cb = addSetting("Climb rate next to the aircraft", "map/aircraft_info_climb", w_map, l_map, row, Type::BOOL);
    callbacks.append(cb);
    cb = addSetting("Ground speed next to the aircraft", "map/aircraft_info_speed", w_map, l_map, row, Type::BOOL);
    callbacks.append(cb);
    cb = addSetting("Distance from the ground station", "map/aircraft_info_gcs", w_map, l_map, row, Type::BOOL);
    callbacks.append(cb);
    addHelp("With a ground GPS (ublox2ivy or gpsd2ivy): the distance and the bearing from the ground station to the aircraft.", w_map, l_map, row);
    live_settings << "map/aircraft_info_";     // read at each position of the aircraft

    addSection("Shapes and texts", w_map, l_map, row);
    cb = addSetting("Other aircraft drawn smaller by", "map/size_highlight_factor", w_map, l_map, row, Type::DOUBLE);
    callbacks.append(cb);
    addHelp("The waypoints, lines and circles of the aircraft that are not selected are drawn smaller, so the selected one stands out. 1.5: two thirds of the size; 1: the same size.", w_map, l_map, row);
    cb = addSetting("Text size (circles, papgets)", "map/items_font", w_map, l_map, row, Type::INT);
    callbacks.append(cb);
    cb = addSetting("Drag starts after (px)", "map/move_hyteresis", w_map, l_map, row, Type::INT);
    callbacks.append(cb);
    addHelp("How far the mouse moves, with the button held, before a press becomes a drag: of a waypoint, a circle or the map. A smaller move is a click.", w_map, l_map, row);
    finishTab(l_map, row);
    tabWidget->addTab(w_map, "Map");

    // Tab Waypoints (on the map)
    auto w_wp = new QWidget(tabWidget);
    auto l_wp = new QGridLayout(w_wp);
    row = 0;
    cb = addSetting("Waypoint size", "map/waypoint/size", w_wp, l_wp, row, Type::INT);
    callbacks.append(cb);
    cb = addSetting("Highlight under the mouse", "map/waypoint_hover_highlight", w_wp, l_wp, row, Type::STRING_COMBO,
                    QStringList{"Name and ring", "Name only", "Off"});
    callbacks.append(cb);
    addHelp("The waypoint or name under the mouse, after a short moment: its name in blue, and a blue ring around the waypoint (Name and ring). Over a waypoint, the names of all the waypoints at that place are highlighted.", w_wp, l_wp, row);
    cb = addSetting("Fade the other names", "map/waypoint_hover_dim", w_wp, l_wp, row, Type::BOOL);
    callbacks.append(cb);
    addHelp("While a waypoint is highlighted, the names of the other waypoints are faded.", w_wp, l_wp, row);
    cb = addSetting("Altitude by the name", "map/waypoint_altitude", w_wp, l_wp, row, Type::STRING_COMBO,
                    QStringList{"On hover|When hovered", "Always|Always", "Off|Never"});
    callbacks.append(cb);
    cb = addSetting("Altitude as", "map/waypoint_altitude_always", w_wp, l_wp, row, Type::STRING_COMBO,
                    QStringList{"Height|Height above the ground", "MSL|Above sea level (MSL)"});
    callbacks.append(cb);
    addHelp("The altitude shown by the names (and in the waypoint card).<br><b>Height above the ground</b>: AGL above the terrain where there is SRTM data, else H above the ground altitude of the flight plan.<br><b>Above sea level</b>: MSL.", w_wp, l_wp, row);

    // used right away (read each time the mouse is over a waypoint, or a drag starts)
    live_settings << "map/waypoint_hover_highlight" << "map/waypoint_hover_dim" << "map/move_hyteresis";
    finishTab(l_wp, row);
    tabWidget->addTab(w_wp, "Waypoints");

    // Tab General
    auto w_general = new QWidget(tabWidget);
    auto l_general = new QGridLayout(w_general);
    row = 0;

    auto user_layout_files = getLayoutFiles("USER_DATA_PATH");
    auto app_layout_files = getLayoutFiles("APP_DATA_PATH");
    cb = addSetting("Layout", "APP_LAYOUT_FILE", w_general, l_general, row, Type::STRING_COMBO, user_layout_files + app_layout_files);
    callbacks.append(cb);
    // the style sheets in the user and application data folders (e.g. default and night),
    // plus the current one if it is elsewhere
    QStringList style_files;
    for(auto location: {"USER_DATA_PATH", "APP_DATA_PATH"}) {
        auto path = appConfig()->value(location).toString();
        if(!path.isEmpty()) {
            for(auto &fn: QDir(path).entryList({"*.qss"})) {
                if(!style_files.contains(fn)) {
                    style_files.append(fn);
                }
            }
        }
    }
    auto current_style = getAppSettings().value("APP_STYLE_FILE").toString();
    if(!current_style.isEmpty() && !style_files.contains(current_style)) {
        style_files.append(current_style);
    }
    cb = addSetting("Style", "APP_STYLE_FILE", w_general, l_general, row, Type::STRING_COMBO, style_files);
    callbacks.append(cb);
    cb = addSetting("Menu bar as the title bar (no window frame)", "window/menu_title_bar", w_general, l_general, row, Type::BOOL);
    callbacks.append(cb);
    finishTab(l_general, row);
    tabWidget->insertTab(0, w_general, "General");

    // Tab Panels
    auto w_panels = new QWidget(tabWidget);
    auto l_panels = new QGridLayout(w_panels);
    row = 0;
    cb = addSetting("Strips: status icons always shown", "strips/status_icons", w_panels, l_panels, row, Type::BOOL);
    callbacks.append(cb);
    cb = addSetting("Settings: every setting open (slider and buttons)", "settings/rows_open", w_panels, l_panels, row, Type::BOOL);
    callbacks.append(cb);
    cb = addSetting("Settings: ask the aircraft for their values", "settings/request_values", w_panels, l_panels, row, Type::BOOL);
    callbacks.append(cb);
    addHelp("When an aircraft is loaded, the values of its settings that are still unknown are asked "
            "(after a reboot: all of them), 10 per second, instead of waiting for the aircraft to send "
            "them one by one (about one per second). Turn it off with a very slow datalink.", w_panels, l_panels, row);
    live_settings << "settings/request_values";     // read when an aircraft is loaded
    finishTab(l_panels, row);
    tabWidget->addTab(w_panels, "Panels");

    // Tab Notifications: which ones show at the top of the map
    auto w_notify = new QWidget(tabWidget);
    auto l_notify = new QGridLayout(w_notify);
    row = 0;
    // a section per group; per kind: its name, when it shows (in grey), how serious (a label
    // per level, as on the map) and its switch
    // the levels (a label per level, as on the map), then the switch: in line from row to row
    auto alignSwitch = [&](QStringList levels) {
        auto item = l_notify->itemAtPosition(row - 1, 1);
        auto sw = item != nullptr ? item->widget() : nullptr;
        if(sw == nullptr) {
            return;
        }
        l_notify->removeWidget(sw);
        auto box = new QWidget(w_notify);
        auto hl = new QHBoxLayout(box);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(4);
        auto badges = new QWidget(box);
        auto ll = new QHBoxLayout(badges);
        ll->setContentsMargins(0, 0, 0, 0);
        ll->setSpacing(4);
        for(auto &level: levels) {
            auto badge = new QLabel(level, badges);
            badge->setObjectName("settings_level_" + level.toLower());
            badge->setFixedHeight(18);
            badge->setAlignment(Qt::AlignCenter);
            ll->addWidget(badge);
        }
        ll->addStretch(1);
        badges->setFixedWidth(140);
        hl->addWidget(badges);
        hl->addWidget(sw);
        hl->addStretch(1);
        l_notify->addWidget(box, row - 1, 1);
    };
    // a section per group, each group once (in the order of its first kind, whatever the order of the kinds)
    QStringList groups;
    for(auto &k: MapNotifications::kinds()) {
        if(!groups.contains(k.group)) {
            groups << k.group;
        }
    }
    QString muted = "#6e7781";     // a grey readable in the light and the night style
    for(auto &group: groups) {
        addSection(group, w_notify, l_notify, row);
        for(auto &k: MapNotifications::kinds()) {
            if(k.group != group) {
                continue;
            }
            QString name = k.name.toHtmlEscaped();
            if(!k.detail.isEmpty()) {
                name += QString("<br><span style='color:%1; font-size:11px;'>%2</span>").arg(muted, k.detail.toHtmlEscaped());
            }
            cb = addSetting(name, "notify/" + k.key, w_notify, l_notify, row, Type::BOOL);
            callbacks.append(cb);
            alignSwitch(k.levels);
        }
    }
    live_settings << "notify/";     // read each time a notification comes
    finishTab(l_notify, row);
    tabWidget->addTab(w_notify, "Notifications");

    // Tab Sound: how the notifications are heard (a sound with each warning or alarm, the speech)
    auto w_sound = new QWidget(tabWidget);
    auto l_sound = new QGridLayout(w_sound);
    row = 0;
    cb = addSetting("Alert sounds", "notify/sound", w_sound, l_sound, row, Type::BOOL);
    callbacks.append(cb);
    addHelp("A chime when a warning of the map shows or gets worse. An alarm beeps, and again every 4 s until it "
            "clears or is dismissed (\u00d7).", w_sound, l_sound, row);
    cb = addSetting("Speech", "notify/speech", w_sound, l_sound, row, Type::BOOL);
    callbacks.append(cb);
#if defined(SPEECH_ENABLED)
    addHelp("Says the warnings and alarms, and the messages of the layout (e.g. the altitude, a new mode or block).",
            w_sound, l_sound, row);
#else
    if(auto item = l_sound->itemAtPosition(row - 1, 1)) {
        item->widget()->setEnabled(false);
    }
    addHelp("This PprzGCS is built without speech (cmake -DSPEECH=ON, with Qt 6.4 or later).", w_sound, l_sound, row);
#endif
    finishTab(l_sound, row);
    tabWidget->addTab(w_sound, "Sound");
    tabWidget->tabBar()->moveTab(tabWidget->indexOf(w_ivy), tabWidget->count() - 1);   // Ivy last

    // each tab scrolls: the dialog as tall as its tallest tab, but no taller than the screen
    QSize largest;
    for(int i=0; i<tabWidget->count(); i++) {
        auto page = tabWidget->widget(i);
        auto text = tabWidget->tabText(i);
        largest = largest.expandedTo(page->sizeHint());
        tabWidget->removeTab(i);
        auto scroll = new QScrollArea(tabWidget);
        scroll->setObjectName("settings_scroll");
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setStyleSheet("#settings_scroll, #settings_scroll > QWidget > QWidget {background: transparent;}");
        scroll->setWidget(page);
        tabWidget->insertTab(i, scroll, text);
    }
    tabWidget->setCurrentIndex(0);


    // Cancel and Save, as in the card dialogs
    auto buttons = new QHBoxLayout();
    buttons->addStretch(1);
    auto cancel_button = new QPushButton("Cancel", this);
    cancel_button->setObjectName("card_button");
    auto save_button = new QPushButton("Save", this);
    save_button->setObjectName("card_button_primary");
    save_button->setDefault(true);
    cancel_button->setMinimumWidth(110);
    save_button->setMinimumWidth(110);
    buttons->addWidget(cancel_button);
    buttons->addWidget(save_button);
    lay->addLayout(buttons);
    setMinimumWidth(560);
    // the size of the largest tab (and its scroll bar), within 85% of the screen
    QSize scrolls;
    for(int i=0; i<tabWidget->count(); i++) {
        scrolls = scrolls.expandedTo(tabWidget->widget(i)->sizeHint());
    }
    QSize want = sizeHint() - scrolls + largest + QSize(style()->pixelMetric(QStyle::PM_ScrollBarExtent), 0);
    if(auto s = screen()) {
        want = want.boundedTo(s->availableGeometry().size() * 0.85);
    }
    resize(want);

    connect(save_button, &QPushButton::clicked, this, [=,this](){
        // what changed: a restart only for the settings that are not used right away
        auto values = [](QSettings& st) {
            QMap<QString, QVariant> v;
            for(auto &k: st.allKeys()) {
                v[k] = st.value(k);
            }
            return v;
        };
        auto before_settings = getAppSettings();
        auto before = values(before_settings);
        for(auto &cb:callbacks) {
            cb();
        }
        auto after_settings = getAppSettings();
        auto after = values(after_settings);
        bool needs_restart = false;
        for(auto it = after.cbegin(); it != after.cend(); ++it) {
            if(before.value(it.key()) == it.value()) {
                continue;
            }
            bool live = false;
            for(auto &prefix: live_settings) {
                live = live || it.key().startsWith(prefix);
            }
            needs_restart = needs_restart || !live;
        }
        if(standalone) {
            // (pprzgcs -c) the GCS starts with them
            accept();
            PprzMain::launch_type = NORMAL;
            qApp->exit( PprzMain::EXIT_CODE_REBOOT );
            return;
        }
        emit DispatcherUi::get()->appSettingsChanged();
        if(!needs_restart) {
            accept();
            return;
        }

        bool restart = CardDialog::ask(this, "Restart PprzGCS?",
                                       "Some of the new settings are used after a restart.", "Restart now",
                                       CardDialog::PRIMARY, "Later");
        accept();
        if(restart) {
            qDebug() << "Restarting application...";
            PprzMain::launch_type = NORMAL;
            qApp->exit( PprzMain::EXIT_CODE_REBOOT );
        }
    });

    connect(cancel_button, &QPushButton::clicked, this, [=,this](){
        reject();
        if(standalone) {
            qDebug() << "Restarting application...";
            PprzMain::launch_type = NORMAL;
            qApp->exit( PprzMain::EXIT_CODE_REBOOT );
        }
    });
}


/// A heading for the rows below it (e.g. "Waypoints")
void SettingsEditor::addSection(QString title, QWidget* w, QGridLayout* gl, int &row) {
    auto label = new QLabel(title, w);
    label->setStyleSheet(QString("font-weight: bold; %1").arg(row > 0 ? "margin-top: 10px;" : ""));
    gl->addWidget(label, row, 0, 1, 3);
    row += 1;
}

/// A "?" next to the setting above: its help, shown when clicked
void SettingsEditor::addHelp(QString text, QWidget* w, QGridLayout* gl, int &row) {
    auto button = new QToolButton(w);
    button->setText("?");
    button->setAutoRaise(true);
    button->setCursor(Qt::WhatsThisCursor);
    button->setStyleSheet("QToolButton { font-weight: bold; color: palette(placeholder-text); "
                          "border: 1px solid palette(mid); border-radius: 9px; min-width: 16px; max-width: 16px; "
                          "min-height: 16px; max-height: 16px; padding: 0px; }");
    // right after a switch (it does not take the width), else after the field
    auto item = gl->itemAtPosition(row - 1, 1);
    auto sw = item != nullptr ? qobject_cast<Switch*>(item->widget()) : nullptr;
    if(sw != nullptr) {
        gl->removeWidget(sw);
        auto box = new QWidget(w);
        auto hl = new QHBoxLayout(box);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(10);
        hl->addWidget(sw);
        hl->addWidget(button);
        hl->addStretch(1);
        gl->addWidget(box, row - 1, 1);
    } else {
        gl->addWidget(button, row - 1, 2, Qt::AlignLeft | Qt::AlignVCenter);
    }
    connect(button, &QToolButton::clicked, button, [=]() {
        QWhatsThis::showText(button->mapToGlobal(QPoint(button->width() / 2, button->height())), text, button);
    });
}

/// the rows at the top with even spacing, the fields taking the width
void SettingsEditor::finishTab(QGridLayout* gl, int row) {
    gl->setContentsMargins(16, 16, 16, 16);
    gl->setHorizontalSpacing(16);
    gl->setVerticalSpacing(10);
    gl->setColumnStretch(1, 1);
    gl->setRowStretch(row, 1);
}

std::function<void()> SettingsEditor::addSetting(QString name, QString key, QWidget* w, QGridLayout* gl, int &row, Type type, QVariant data) {
    auto settings = getAppSettings();
    auto label = new QLabel(name, w);

    int r = row;
    row += 1;


    gl->addWidget(label, r, 0);

    if(type == Type::STRING) {
        auto edit = new QLineEdit(settings.value(key).toString(), w);
        gl->addWidget(edit, r, 1);

        auto cb = [=,this]() {
          auto settings = getAppSettings();
          settings.setValue(key, edit->text());
        };

        return cb;
    } else if(type == Type::INT) {
        auto edit = new QLineEdit(settings.value(key).toString(), w);
        gl->addWidget(edit, r, 1);

        auto cb = [=,this]() {
          auto settings = getAppSettings();
          settings.setValue(key, edit->text().toInt());
        };

        return cb;
    } else if(type == Type::DOUBLE) {
        auto edit = new QLineEdit(settings.value(key).toString(), w);
        gl->addWidget(edit, r, 1);

        auto cb = [=,this]() {
          auto settings = getAppSettings();
          settings.setValue(key, edit->text().toDouble());
        };

        return cb;
    } else if(type == Type::PATH_DIR) {
        auto edit = new QLineEdit(settings.value(key).toString(), w);
        gl->addWidget(edit, r, 1);

        QToolButton* but = new QToolButton(w);
        but->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
        gl->addWidget(but, r, 2);
        connect(but, &QToolButton::clicked, this, [=,this]() {
            auto dir = QFileDialog::getExistingDirectory(w, name, edit->text());
            qDebug() << dir;
            if(dir != "") {
                edit->setText(dir);
            }
        });

        auto cb = [=,this]() {
          auto settings = getAppSettings();
          settings.setValue(key, edit->text());
        };

        return cb;
    }  else if(type == Type::PATH_FILE) {
        auto edit = new QLineEdit(settings.value(key).toString(), w);
        gl->addWidget(edit, r, 1);

        QToolButton* but = new QToolButton(w);
        but->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
        gl->addWidget(but, r, 2);
        connect(but, &QToolButton::clicked, this, [=,this]() {
            QFileInfo fi(edit->text());
            auto dir = QFileDialog::getOpenFileName(w, name, fi.absoluteDir().path());
            qDebug() << dir;
            if(dir != "") {
                edit->setText(dir);
            }
        });

        auto cb = [=,this]() {
          auto settings = getAppSettings();
          settings.setValue(key, edit->text());
        };

        return cb;
    } else if(type == Type::STRING_COMBO) {
        // an item "value|text": the text shown, the value stored
        auto combo = new QComboBox(w);
        for(auto &item: data.toStringList()) {
            auto parts = item.split('|');
            combo->addItem(parts.value(1, parts[0]), parts[0]);
        }
        int current = combo->findData(settings.value(key).toString());
        if(current >= 0) {
            combo->setCurrentIndex(current);
        }
        gl->addWidget(combo, r, 1);

        auto cb = [=,this]() {
            auto settings = getAppSettings();
            settings.setValue(key, combo->currentData().toString());
        };

        return cb;
    } else if(type == Type::BOOL) {
        auto check = new Switch(w);
        check->setChecked(settings.value(key).toBool());
        gl->addWidget(check, r, 1, Qt::AlignLeft);
        auto cb = [=,this]() {
            auto settings = getAppSettings();
            settings.setValue(key, check->isChecked());
        };
        return cb;
    }
    else {
        throw std::runtime_error("Error: Setting type not handled !");
    }
}
