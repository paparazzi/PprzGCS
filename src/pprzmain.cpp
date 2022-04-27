#include "pprzmain.h"
#include <QMessageBox>
#include <QDebug>
#include "dispatcher_ui.h"
#include "AircraftManager.h"
#include "pprz_dispatcher.h"
#include <QSettings>
#include "app_settings.h"
#include "gcs_utils.h"

#if defined(SPEECH_ENABLED)
#include "speaker.h"
#endif

LaunchTypes PprzMain::launch_type = DEFAULT;

PprzMain::PprzMain(QWidget *parent) :
    QMainWindow(parent)
{

}

void PprzMain::setupUi(int width, int height, QWidget* centralWidget) {
    centralWidget->setParent(this);
    resize(width, height);
    menuBar = new QMenuBar(this);
    menuBar->setObjectName(QString::fromUtf8("menuBar"));
    menuBar->setGeometry(QRect(0, 0, 555, 22));
    setMenuBar(menuBar);
    mainToolBar = new QToolBar(this);
    mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
    addToolBar(Qt::TopToolBarArea, mainToolBar);
    statusBar = new QStatusBar(this);
    statusBar->setObjectName(QString::fromUtf8("statusBar"));
    setStatusBar(statusBar);
    setCentralWidget(centralWidget);
    setWindowIcon(QIcon(":/pictures/icon.svg"));
    populate_menu();

    statusBar->addPermanentWidget(new QLabel("server status:"));

    serverStatusLed = new QLabel(statusBar);
    setServerStatus(false);
    statusBar->addPermanentWidget(serverStatusLed);

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &PprzMain::newAC);
    connect(DispatcherUi::get(), &DispatcherUi::ac_deleted, this, &PprzMain::removeAC);
}

void PprzMain::setServerStatus(bool active) {
    QIcon ic;
    if(active) {
        ic = QIcon(":/pictures/green_led.svg");
    } else {
        ic = QIcon(":/pictures/red_led.svg");
    }
    serverStatusLed->setPixmap(ic.pixmap(15, 15));
}

void PprzMain::populate_menu() {

    auto file_menu = menuBar->addMenu("&File");

    auto user_dir = file_menu->addAction("Open user directory");
    connect(user_dir, &QAction::triggered, [=, this](){
        auto settings = getAppSettings();
        QString path = QDir::toNativeSeparators(appConfig()->value("USER_DATA_PATH").toString());
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });

    auto app_dir = file_menu->addAction("Open app directory");
    connect(app_dir, &QAction::triggered, [=, this](){
        auto settings = getAppSettings();
        QString path = QDir::toNativeSeparators(appConfig()->value("APP_DATA_PATH").toString());
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });

    auto edit_settings = file_menu->addAction("Edit Settings");
    connect(edit_settings, &QAction::triggered, [=, this](){
        auto se = new SettingsEditor();
        se->open();
    });

    auto silent_mode_action = file_menu->addAction("Silent mode");
    silent_mode_action->setCheckable(true);
    silent_mode_action->setChecked(PprzDispatcher::get()->isSilent());

    auto speech_action = file_menu->addAction("Enable Speech");
    speech_action->setCheckable(true);
    speech_action->setChecked(speech());

    auto map_track_ac = file_menu->addAction("Track AC");
    map_track_ac->setCheckable(true);
    map_track_ac->setChecked(false);

    auto open_flight_plan = file_menu->addAction("Open FlightPlans");
    connect(open_flight_plan, &QAction::triggered, [=, this](){
        auto settings = getAppSettings();
        auto pprz_home = appConfig()->value("PAPARAZZI_HOME").toString();
        auto files = QFileDialog::getOpenFileNames(this, "open fp", pprz_home + "/conf/flight_plans", "*.xml");
        if(PprzMain::launch_type != FLIGHTPLAN_EDIT) {
            PprzMain::launch_type = FLIGHTPLAN_EDIT;
            appConfig()->setValue("FLIGHTPLAN_FILES", files);
            qApp->exit( PprzMain::EXIT_CODE_REBOOT );
        } else {
            for(auto &fp_file: files) {
                qDebug() << "edit flightplan " << fp_file;
                auto name = fp_file.split("/").last();
                AircraftManager::get()->addFPAircraft(name, fp_file);
            }
        }
    });

    connect(silent_mode_action, &QAction::toggled, [=, this](bool checked) {
        PprzDispatcher::get()->setSilent(checked);
    });

#if defined(SPEECH_ENABLED)
    connect(speech_action, &QAction::toggled, pprzApp()->toolbox()->speaker(), &Speaker::enableSpeech);
#endif

    connect(map_track_ac, &QAction::toggled, [=, this](bool checked) {
        GlobalConfig::get()->setValue("MAP_TRACK_AC", checked);
    });

    auto quit = file_menu->addAction("&Quit");
    connect(quit, &QAction::triggered, qApp, QApplication::quit);


    aircraftsTopMenu = menuBar->addMenu("&Aircrafts");

    aircraftsTopMenu->addAction("Update", PprzDispatcher::get(), &PprzDispatcher::requestAircrafts);
    auto show_hidden_wp_action = aircraftsTopMenu->addAction("Show hidden waypoints");
    show_hidden_wp_action->setCheckable(true);

    connect(show_hidden_wp_action, &QAction::toggled, [=, this](bool show) {
        emit DispatcherUi::get()->showHiddenWaypoints(show);
    });

    auto help_menu = menuBar->addMenu("&Help");
    auto about = help_menu->addAction("&About");

    QString about_txt =  QString(
            "<h1>PprzGCS</h1>"
            "version %1"
            "<p>PprzGCS is a ground control software for <a href=\"https://paparazziuav.org\">paparrazi UAV</a> drones.</p>"
            "<p>source code can be found here: "
            "<a href=\"https://github.com/paparazzi/PprzGCS\">https://github.com/paparazzi/PprzGCS</a></p>"
            "<p>Copyright (C) 2019-2022 Fabien Bonneval (fabien.bonneval@gmail.com)</p>"
            "<p>PprzGCS is free software; you can redistribute it and/or modify "
            "it under the terms of the <a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">GNU General Public License</a> as published by "
            "the Free Software Foundation; either version 3, or (at your option) "
            "any later version.</p>"
            "<p>The program is provided AS IS with "
            "NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, "
            "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.</p>"
            "<h2>Third-party libraries</h2>"
            "<ul>"
            "<li><a href=\"https://www.qt.io/\">Qt 5.12.5</a></li>"
            "<li><a href=\"https://proj.org/\">Proj</a></li>"
            "<li><a %1 href=\"https://libzip.org/\">libzip</a></li>"
            "</ul>").arg(QCoreApplication::applicationVersion());

    connect(about, &QAction::triggered, [=, this]() {
        QMessageBox::about(this,"About PprzGCS", about_txt);

    });

    auto doc = help_menu->addAction("&Documentation");

    connect(doc, &QAction::triggered, [=, this]() {
        QDesktopServices::openUrl(QUrl("https://docs.paparazziuav.org/PprzGCS/"));
    });

}


void PprzMain::newAC(QString ac_id) {
    QString acName = AircraftManager::get()->getAircraft(ac_id)->name();
    auto menu = new QMenu("&" + acName);
    auto action = aircraftsTopMenu->addMenu(menu);

    acMenus[ac_id] = menu;
    acActions[ac_id] = action;

    menu->addAction("Remove", this, [ac_id](){
        AircraftManager::get()->removeAircraft(ac_id);
    });
}

void PprzMain::removeAC(QString ac_id) {
    auto action = acActions[ac_id];
    auto menu = acMenus[ac_id];

    acActions.remove(ac_id);
    acMenus.remove(ac_id);

    aircraftsTopMenu->removeAction(action);

    menu->deleteLater();
    action->deleteLater();

}
