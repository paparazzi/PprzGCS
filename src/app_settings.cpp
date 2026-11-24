#if defined(AUDIO_RECORDER_ENABLED)
#include "audio_recorder.h"
#include "audio_mic_settings.h"
#include "camera_settings.h"
#include "video_recorder.h"
#endif
#include "card_dialog.h"
#include "app_settings.h"
#include "dispatcher_ui.h"
#include "map_notifications.h"
#include "speaker.h"
#include "alert_sound.h"
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
#include <QComboBox>
#include <QPushButton>
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
    // the ground station: the yellow of the old icon (ring and cross) is orange for the new ones
    // (and its old size, 16: a bit smaller)
    if(!getAppSettings().contains("map/gcs_icon_style")) {
        if(getAppSettings().value("map/gcs_icon_color").toString() == "#ffff00") {
            getAppSettings().setValue("map/gcs_icon_color", "#e8a33d");
        }
        if(getAppSettings().value("map/gcs_icon_size").toInt() == 16) {
            getAppSettings().setValue("map/gcs_icon_size", 12);
        }
    }
    default_setting("map/gcs_icon_style", "marker");
    default_setting("map/gcs_icon_size", 12);
    default_setting("map/gcs_icon_color", "#e8a33d");
    default_setting("map/gcs_track", true);
    default_setting("map/gcs_track_seconds", 300);
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
    default_setting("notify/output", "");
    default_setting("notify/volume", 100);
    default_setting("audio/enabled", false);
    default_setting("audio/name", "");
    default_setting("audio/format", "opus");
    default_setting("audio/folder", "");
    default_setting("audio/chunk_minutes", 10);
    default_setting("audio/mark_key", "F9");
    default_setting("video/folder", "");
    default_setting("notify/speech", false);
    default_setting("notify/speech_voice", "");
    default_setting("notify/speech_rate", "0");
    default_setting("APP_STYLE_FILE", "default_style.qss");
    default_setting("APP_LAYOUT_FILE", "default_layout.xml");
    default_setting("groups_conf", "swarm.xml");

}



SettingsEditor::SettingsEditor(bool standalone, QWidget* parent): QDialog(parent)
{
    setWindowTitle("PprzGCS application settings");
    auto lay = new QVBoxLayout(this);

    auto tabWidget = new QTabWidget(this);
    tabs = tabWidget;
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

    addSection("Ground station", w_map, l_map, row);
    addHelp("With a ground GPS (ublox2ivy or gpsd2ivy). With two antennas (ublox2ivy), the icon shows the heading.", w_map, l_map, row);
    cb = addSetting("Icon", "map/gcs_icon_style", w_map, l_map, row, Type::STRING_COMBO,
                    QStringList{"marker|Marker", "boat|Boat", "car|Car"});
    callbacks.append(cb);
    cb = addSetting("Color", "map/gcs_icon_color", w_map, l_map, row, Type::STRING_COMBO,
                    QStringList{"#e8a33d|Orange", "#ffff00|Yellow", "#1f6feb|Blue", "#e5484d|Red", "#2da44e|Green", "#ffffff|White"});
    callbacks.append(cb);
    cb = addSetting("Size", "map/gcs_icon_size", w_map, l_map, row, Type::INT);
    callbacks.append(cb);
    cb = addSetting("Track", "map/gcs_track", w_map, l_map, row, Type::BOOL);
    callbacks.append(cb);
    cb = addSetting("Track length (s)", "map/gcs_track_seconds", w_map, l_map, row, Type::INT);
    callbacks.append(cb);
    live_settings << "map/gcs_";     // read at each position of the ground station

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
    auto group_files = getXmlFiles("USER_DATA_PATH", "swarm") + getXmlFiles("APP_DATA_PATH", "swarm");
    group_files.removeDuplicates();
    auto current_groups = settings.value("groups_conf").toString();
    if(!current_groups.isEmpty() && !group_files.contains(current_groups)) {
        group_files.append(current_groups);
    }
    cb = addSetting("Groups configuration file", "groups_conf", w_general, l_general, row, Type::STRING_COMBO, group_files);
    callbacks.append(cb);
    addHelp("Swarm XML file defining drone groups, statuses and actions. "
            "The --swarm-conf command-line option overrides this file.", w_general, l_general, row);
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
    cb = addSetting("Mark key", "audio/mark_key", w_general, l_general, row, Type::STRING);
    callbacks.append(cb);
    addHelp("The key of the Mark button (status bar): a mark in the logs and the Messages at this moment, to find "
            "it back in the flight log, the audio and the transcript. A function key (e.g. F9) works also while "
            "typing in a text field. Empty: no key.", w_general, l_general, row);
    live_settings << "audio/mark_key";     // the key changes right away
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
            "clears or is dismissed (\u00d7). Their output (e.g. an intercom for the crew) and volume: the Recording "
            "tab, per profile.", w_sound, l_sound, row);
    cb = addSetting("Speech", "notify/speech", w_sound, l_sound, row, Type::BOOL);
    callbacks.append(cb);
    if(Speaker::available()) {
        addHelp("Says the warnings and alarms, and the messages of the layout (e.g. the altitude, a new mode or block).",
                w_sound, l_sound, row);

        // the voices of speech-dispatcher (spd-say): each engine installed (espeak-ng, pico, rhvoice...)
        cb = addSetting("Voice", "notify/speech_voice", w_sound, l_sound, row, Type::STRING_COMBO,
                        QStringList{"|Default"} + Speaker::voices());
        callbacks.append(cb);
        auto voice_combo = qobject_cast<QComboBox*>(l_sound->itemAtPosition(row - 1, 1)->widget());
        addHelp("Better voices than espeak-ng: sudo apt install speech-dispatcher-pico (or speech-dispatcher-rhvoice "
                "rhvoice-english), then systemctl --user restart speech-dispatcher.", w_sound, l_sound, row);
        cb = addSetting("Rate", "notify/speech_rate", w_sound, l_sound, row, Type::STRING_COMBO,
                        QStringList{"-40|Slower", "-20|Slow", "0|Normal", "25|Fast", "50|Faster"});
        callbacks.append(cb);
        auto rate_combo = qobject_cast<QComboBox*>(l_sound->itemAtPosition(row - 1, 1)->widget());
        // said with the voice and rate chosen, before they are saved
        auto try_button = new QPushButton("Try", w_sound);
        l_sound->addWidget(try_button, row - 1, 2);
        connect(try_button, &QPushButton::clicked, this, [=]() {
            Speaker::trySay("Welcome to Paparazzi. Altitude 120 meters.", voice_combo->currentData().toString(),
                            rate_combo->currentData().toInt());
        });
    } else {
        if(auto item = l_sound->itemAtPosition(row - 1, 1)) {
            item->widget()->setEnabled(false);
        }
        addHelp("Nothing to speak with: install speech-dispatcher (spd-say) or espeak-ng.", w_sound, l_sound, row);
    }
    finishTab(l_sound, row);
    tabWidget->addTab(w_sound, "Sound");
    // Tab Recording: the microphones (e.g. of the pilots), for the transcription and the logbook
    auto w_rec = new QWidget(tabWidget);
    auto l_rec = new QGridLayout(w_rec);
    row = 0;
#if defined(AUDIO_RECORDER_ENABLED)
    addSection("Recording", w_rec, l_rec, row);
    cb = addSetting("Record the microphones and cameras", "audio/enabled", w_rec, l_rec, row, Type::BOOL);
    callbacks.append(cb);
    addHelp("The microphones below are recorded in var/logs/audio (or the folder below), a file per microphone, for the "
            "transcription and the logbook after the flight. The status bar shows the recording, the level of "
            "each channel, and a microphone that is not connected or has no signal.<br>Started from the status "
            "bar (click the recorder), or from the start when PprzGCS runs with --record: e.g. in the flight "
            "sessions of the Paparazzi Center, not in the simulations. It records until PprzGCS closes, and "
            "starts again after a crash during a recording. Never while editing flight plans.", w_rec, l_rec, row);
    cb = addSetting("Format", "audio/format", w_rec, l_rec, row, Type::STRING_COMBO,
                    QStringList{"opus|Opus (speech, about 11 MB per channel and hour)",
                                "flac|FLAC (lossless, about 15 times larger)"});
    callbacks.append(cb);
    cb = addSetting("New file every (minutes)", "audio/chunk_minutes", w_rec, l_rec, row, Type::INT);
    callbacks.append(cb);
    addHelp("A crash or a power loss then loses only the last seconds. The files follow each other "
            "without a gap.", w_rec, l_rec, row);
    cb = addSetting("Folder (empty: var/logs/audio)", "audio/folder", w_rec, l_rec, row, Type::PATH_DIR);
    callbacks.append(cb);
    cb = addSetting("Recorder name (empty: the computer)", "audio/name", w_rec, l_rec, row, Type::STRING);
    callbacks.append(cb);
    addHelp("Shown in the status bar of the other GCSs, which can start and stop it.", w_rec, l_rec, row);
    cb = addSetting("Video folder (empty: var/logs/video)", "video/folder", w_rec, l_rec, row, Type::PATH_DIR);
    callbacks.append(cb);
    addHelp("Recorded with the microphones (ffmpeg): a USB camera in H.264, a stream as it comes, a file per camera "
            "and a new one with the audio files. About 0.5 to 1.5 GB per camera and hour. The Video panel shows "
            "them (right click: the source).", w_rec, l_rec, row);

    live_settings << "video/";
#else
    addSection("Recording", w_rec, l_rec, row);
    addHelp("This PprzGCS is built without the audio recorder (install qt6-multimedia-dev and libsndfile1-dev, "
            "and build again).", w_rec, l_rec, row);
#endif

    // The devices of a place (profile): the microphones, the cameras, the output of the alerts.
    // Edited here for each profile, saved together; pprzgcs --profile NAME uses one (else the default).
    struct ProfileEdit {
#if defined(AUDIO_RECORDER_ENABLED)
        QList<MicConfig> mics;
        QList<CameraConfig> cams;
#endif
        QString output;
        int volume = 100;
    };
    struct ProfileState {
        QString current;                    // "": the default profile
        QMap<QString, ProfileEdit> edits;   // the profiles shown in the dialog (by name)
        QStringList deleted;
    };
    auto pstate = std::make_shared<ProfileState>();
    pstate->current = activeProfile();

    addSection("Profile", w_rec, l_rec, row);
    l_rec->addWidget(new QLabel("Devices of", w_rec), row, 0);
    auto prof_box = new QWidget(w_rec);
    auto prof_lay = new QHBoxLayout(prof_box);
    prof_lay->setContentsMargins(0, 0, 0, 0);
    auto prof_combo = new QComboBox(prof_box);
    auto prof_new = new QPushButton("New...", prof_box);
    prof_new->setToolTip("A new profile, starting as a copy of this one");
    auto prof_delete = new QPushButton("Delete", prof_box);
    prof_lay->addWidget(prof_combo, 1);
    prof_lay->addWidget(prof_new);
    prof_lay->addWidget(prof_delete);
    l_rec->addWidget(prof_box, row, 1);
    row += 1;
    {
        QString help = "The microphones, cameras and alert output of a place (e.g. cyberzoo, field), set up on each "
                       "computer. A session of the Paparazzi Center chooses one with pprzgcs --profile NAME (e.g. "
                       "--profile cyberzoo); without it, or when this computer has no profile of that name, the "
                       "default one.";
        if(!requestedProfile().isEmpty()) {
            help += activeProfile().isEmpty() ?
                        QString("<br>This run asked for profile %1, which this computer does not have: the default "
                                "one is used.").arg(requestedProfile().toHtmlEscaped()) :
                        QString("<br>This run uses profile %1.").arg(activeProfile().toHtmlEscaped());
        }
        addHelp(help, w_rec, l_rec, row);
    }

#if defined(AUDIO_RECORDER_ENABLED)
    addSection("Microphones", w_rec, l_rec, row);
    auto mics = new AudioMicSettings(w_rec);
    l_rec->addWidget(mics, row, 0, 1, 3);
    row += 1;

    addSection("Cameras", w_rec, l_rec, row);
    auto cams = new CameraSettings(w_rec);
    l_rec->addWidget(cams, row, 0, 1, 3);
    row += 1;
#endif

    addSection("Alerts", w_rec, l_rec, row);
    l_rec->addWidget(new QLabel("Output", w_rec), row, 0);
    auto out_combo = new QComboBox(w_rec);
    l_rec->addWidget(out_combo, row, 1);
    row += 1;
    l_rec->addWidget(new QLabel("Volume", w_rec), row, 0);
    auto vol_box = new QWidget(w_rec);
    auto vol_lay = new QHBoxLayout(vol_box);
    vol_lay->setContentsMargins(0, 0, 0, 0);
    auto vol_slider = new QSlider(Qt::Horizontal, vol_box);
    vol_slider->setRange(0, 100);
    auto vol_label = new QLabel(vol_box);
    vol_label->setMinimumWidth(vol_label->fontMetrics().horizontalAdvance("100 %"));
    connect(vol_slider, &QSlider::valueChanged, vol_label, [=]() {
        vol_label->setText(QString("%1 %").arg(vol_slider->value()));
    });
    auto test = new QPushButton("Test", vol_box);
    test->setToolTip("Plays the warning chime on this output, at this volume");
    connect(test, &QPushButton::clicked, this, [=]() {
        AlertSound::play(false, out_combo->currentData().toString(), vol_slider->value());
    });
    vol_lay->addWidget(vol_slider, 1);
    vol_lay->addWidget(vol_label);
    vol_lay->addWidget(test);
    l_rec->addWidget(vol_box, row, 1);
    row += 1;
    addHelp("Where the alert sounds (Sound tab) are heard, e.g. an intercom (Vokkero) for the whole crew: an output "
            "that is not connected (e.g. Bluetooth, off) plays on the default output. The volume is also the one "
            "of the speech.", w_rec, l_rec, row);

    auto loadProfile = [=](QString name) {
        if(pstate->edits.contains(name)) {
            return pstate->edits[name];
        }
        ProfileEdit e;
#if defined(AUDIO_RECORDER_ENABLED)
        e.mics = AudioRecorder::micConfigs(name);
        e.cams = VideoRecorder::cameraConfigs(name);
#endif
        auto settings = getAppSettings();
        e.output = settings.value(profileKey("notify/output", name)).toString();
        e.volume = settings.value(profileKey("notify/volume", name), 100).toInt();
        return e;
    };
    auto storeShown = [=]() {
        ProfileEdit e;
#if defined(AUDIO_RECORDER_ENABLED)
        e.mics = mics->configs();
        e.cams = cams->configs();
#endif
        e.output = out_combo->currentData().toString();
        e.volume = vol_slider->value();
        pstate->edits[pstate->current] = e;
    };
    auto showProfile = [=](QString name) {
        auto e = loadProfile(name);
        pstate->current = name;
#if defined(AUDIO_RECORDER_ENABLED)
        mics->setConfigs(e.mics);
        cams->setConfigs(e.cams);
#endif
        out_combo->clear();
        out_combo->addItem("Default output of the system", QString());
        for(auto &o: AlertSound::outputs()) {
            out_combo->addItem(o.description, o.name);
        }
        if(!e.output.isEmpty() && out_combo->findData(e.output) < 0) {
            // kept, until it is connected again (meanwhile: the default)
            out_combo->addItem(e.output + " (not connected)", e.output);
        }
        out_combo->setCurrentIndex(std::max(0, out_combo->findData(e.output)));
        vol_slider->setValue(e.volume);
        vol_label->setText(QString("%1 %").arg(e.volume));
        prof_delete->setEnabled(!name.isEmpty());
    };
    auto fillProfiles = [=]() {
        QSignalBlocker block(prof_combo);
        prof_combo->clear();
        prof_combo->addItem("Default (without --profile)", QString());
        QStringList names = profileNames();
        for(auto &n: pstate->edits.keys()) {
            if(!n.isEmpty() && !names.contains(n)) {
                names.append(n);
            }
        }
        for(auto &n: pstate->deleted) {
            names.removeAll(n);
        }
        names.sort(Qt::CaseInsensitive);
        for(auto &n: names) {
            prof_combo->addItem(n == activeProfile() && !n.isEmpty() ? n + " (this run)" : n, n);
        }
        prof_combo->setCurrentIndex(std::max(0, prof_combo->findData(pstate->current)));
    };
    fillProfiles();
    showProfile(pstate->current);

    connect(prof_combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [=]() {
        storeShown();
        showProfile(prof_combo->currentData().toString());
    });
    connect(prof_new, &QPushButton::clicked, this, [=]() {
        bool ok = false;
        QString name = QInputDialog::getText(this, "New profile", "Name (letters, digits, - and _), e.g. cyberzoo:",
                                             QLineEdit::Normal, QString(), &ok).trimmed();
        if(!ok || name.isEmpty()) {
            return;
        }
        bool taken = name.compare("default", Qt::CaseInsensitive) == 0;
        for(int i = 1; i < prof_combo->count(); i++) {
            taken |= prof_combo->itemData(i).toString().compare(name, Qt::CaseInsensitive) == 0;
        }
        if(!QRegularExpression("^[A-Za-z0-9_-]+$").match(name).hasMatch() || taken) {
            QMessageBox::warning(this, "New profile", taken ? "There is already a profile of that name." :
                                     "Only letters, digits, - and _ (it is passed as --profile NAME).");
            return;
        }
        storeShown();
        pstate->edits[name] = pstate->edits[pstate->current];   // a copy of the shown one
        pstate->deleted.removeAll(name);
        pstate->current = name;
        fillProfiles();
        showProfile(name);
    });
    connect(prof_delete, &QPushButton::clicked, this, [=]() {
        QString name = pstate->current;
        if(name.isEmpty()) {
            return;
        }
        if(QMessageBox::question(this, "Delete profile", QString("Delete profile %1 (its microphones, cameras and "
                                 "alert output) when saving?").arg(name)) != QMessageBox::Yes) {
            return;
        }
        pstate->edits.remove(name);
        pstate->deleted.append(name);
        pstate->current = QString();
        fillProfiles();
        showProfile(QString());
    });
    callbacks.append([=]() {
        storeShown();
        auto settings = getAppSettings();
        for(auto &n: pstate->deleted) {
            settings.remove("profiles/" + n);
        }
        for(auto it = pstate->edits.constBegin(); it != pstate->edits.constEnd(); ++it) {
#if defined(AUDIO_RECORDER_ENABLED)
            AudioRecorder::saveMicConfigs(it->mics, it.key());
            VideoRecorder::saveCameraConfigs(it->cams, it.key());
#endif
            settings.setValue(profileKey("notify/output", it.key()), it->output);
            settings.setValue(profileKey("notify/volume", it.key()), it->volume);
        }
    });
    live_settings << "profiles/";
    live_settings << "audio/";      // the recorder follows right away
    finishTab(l_rec, row);
    tabWidget->addTab(w_rec, "Recording");

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


void SettingsEditor::showTab(QString name) {
    for(int i=0; i<tabs->count(); i++) {
        if(tabs->tabText(i) == name) {
            tabs->setCurrentIndex(i);
        }
    }
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
