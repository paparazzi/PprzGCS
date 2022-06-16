#include "pprzmain.h"
#include <QApplication>
#include <iostream>
#include "configurator.h"
#include <QNetworkProxy>
#include <QProcessEnvironment>
#include "pprz_dispatcher.h"
#include <QWizard>
#include "app_settings.h"
#include "gcs_utils.h"
#include <QSettings>
#include "PprzApplication.h"
#include "gcs_utils.h"
#include "AircraftManager.h"
#include "globalconfig.h"
#include "python_plugins.h"

#if defined(SPEECH_ENABLED)
#include "speaker.h"
#endif

#ifndef DEFAULT_APP_DATA_PATH
#error "you need to define DEFAULT_APP_DATA_PATH!"
#endif

#ifndef PPRZGCS_VERSION
#error "you need to define PPRZGCS_VERSION!"
#endif

void launch_main_app() {
    auto settings = getAppSettings();
    QString style_path = user_or_app_path(settings.value("APP_STYLE_FILE").toString());
    QFile file(style_path);
    if(!file.open(QFile::ReadOnly | QFile::Text)) {
        qCritical()  << "could not open " << settings.value("APP_STYLE_FILE").toString();
        exit(-1);
    }
    QTextStream stream(&file);
    qApp->setStyleSheet(stream.readAll());

    QString layout_path;

    switch (PprzMain::launch_type) {
    case DEFAULT:
    case NORMAL:
        layout_path = user_or_app_path(settings.value("APP_LAYOUT_FILE").toString());
        break;
    case FLIGHTPLAN_EDIT:
        layout_path = user_or_app_path("fp_editor_layout.xml");
        break;
    case CONFIGURE:
        break;
    case QUIT:
        break;
    }

    if(PprzMain::launch_type == CONFIGURE) {
        auto setedit = new SettingsEditor(true);
        setedit->show();
    } else {
        PprzMain* w = configure(layout_path);
        pprzApp()->toolbox()->plugins()->bind_main_window(w);
        PprzDispatcher::get()->start();
        w->show();
    }

}

int main(int argc, char *argv[])
{
    qSetMessagePattern("[%{category}] %{file}:%{line} %{message}");

    int return_code = 0;
    do {
        auto gconfig = GlobalConfig::get();

        QCoreApplication::setApplicationName("pprzgcs");
        QCoreApplication::setApplicationVersion(PPRZGCS_VERSION);

        auto settings_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/settings.conf";
        gconfig->setValue("SETTINGS_PATH", settings_path);

        PprzApplication a(argc, argv);

        QCommandLineParser parser;
        parser.setApplicationDescription("Test helper");
        parser.addHelpOption();
        parser.addVersionOption();
        parser.addOption({{"c", "configure"}, "Configure app settings."});
        parser.addOption({{"s", "silent"}, "Silent mode."});
        parser.addOption({{"v", "verbose"}, "Verbose"});
        parser.addOption({{"f", "fpedit"}, "edit flight plan", "file"});
        parser.addOption({{"b", "bus"}, "Ivy bus", "bus"});
#if defined(SPEECH_ENABLED)
        parser.addOption({"speech", "Enable speech"});
#endif
        parser.process(a);

        setVerbose(parser.isSet("v"));

#if defined(SPEECH_ENABLED)
        setSpeech(parser.isSet("speech"));
        pprzApp()->toolbox()->speaker()->enableSpeech(parser.isSet("speech"));
#endif

        if(parser.isSet("fpedit") && PprzMain::launch_type == DEFAULT) {
            PprzMain::launch_type = FLIGHTPLAN_EDIT;
            appConfig()->setValue("FLIGHTPLAN_FILES", parser.values("fpedit"));
        }

        if(parser.isSet("configure") && PprzMain::launch_type == DEFAULT) {
            PprzMain::launch_type = CONFIGURE;
        }

        if(parser.isSet("bus")) {
            appConfig()->setValue("IVY_BUS", parser.value("bus"));
        }

        QString config_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir conf_dir(config_path);
        if(!conf_dir.exists()) {
            conf_dir.mkpath(conf_dir.path());
        }

        gconfig->setValue("USER_DATA_PATH", config_path);


        auto data_path = QString(qgetenv("PPRZGCS_DATA_PATH"));
        if(data_path == "") {
            data_path = DEFAULT_APP_DATA_PATH;
        }
        gconfig->setValue("APP_DATA_PATH", data_path);

        set_app_settings();

        a.init();

        PprzDispatcher::get()->setSilent(parser.isSet("silent"));


        launch_main_app();

        if(PprzMain::launch_type == FLIGHTPLAN_EDIT) {
            auto fp_files = appConfig()->value("FLIGHTPLAN_FILES").toStringList();
            for(auto &fp_file: fp_files) {
                qDebug() << "edit flightplan " << fp_file;
                auto name = fp_file.split("/").last();
                AircraftManager::get()->addFPAircraft(name, fp_file);
            }
        }

        return_code = qApp->exec();
        a._shutdown();
    } while(return_code == PprzMain::EXIT_CODE_REBOOT);
    return return_code;
}

