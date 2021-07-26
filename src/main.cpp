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
#include "globalstate.h"
#include "speaker.h"
#include "globalconfig.h"

#ifndef DEFAULT_APP_DATA_PATH
#error "you need to define DEFAULT_APP_DATA_PATH!"
#endif

void launch_main_app() {
    auto settings = getAppSettings();
    QFile file(settings.value("APP_STYLE_FILE").toString());
    if(!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "could not open " << settings.value("APP_STYLE_FILE").toString();
        exit(-1);
    }
    QTextStream stream(&file);
    qApp->setStyleSheet(stream.readAll());

    QString layout_path;

    switch (PprzMain::launch_type) {
    case DEFAULT:
    case NORMAL:
        layout_path = settings.value("APP_LAYOUT_FILE").toString();
        break;
    case FLIGHTPLAN_EDIT:
        layout_path = settings.value("APP_DATA_PATH").toString() + "/fp_editor_layout.xml";
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
        PprzDispatcher::get()->start();
        w->show();
    }

}

int main(int argc, char *argv[])
{
    int return_code = 0;
    do {
        auto gconfig = GlobalConfig::get();

        auto settings_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/settings.conf";
        gconfig->setValue("SETTINGS_PATH", settings_path);

        PprzApplication a(argc, argv);

        QCoreApplication::setApplicationName("PprzGCS");
        QCoreApplication::setApplicationVersion("0.1");

        QCommandLineParser parser;
        parser.setApplicationDescription("Test helper");
        parser.addHelpOption();
        parser.addVersionOption();
        parser.addOption({{"c", "configure"}, "Configure app settings."});
        parser.addOption({{"s", "silent"}, "Silent mode."});
        parser.addOption({{"v", "verbose"}, "Verbose"});
        parser.addOption({{"f", "fpedit"}, "edit flight plan", "file"});
        parser.addOption({"speech", "Enable speech"});
        parser.process(a);

        setVerbose(parser.isSet("v"));
        setSpeech(parser.isSet("speech"));
        pprzApp()->toolbox()->speaker()->enableSpeech(parser.isSet("speech"));

        if(parser.isSet("fpedit") && PprzMain::launch_type == DEFAULT) {
            PprzMain::launch_type = FLIGHTPLAN_EDIT;
            GlobalState::get()->set("FLIGHTPLAN_FILES", parser.values("fpedit"));
        }

        if(parser.isSet("configure") && PprzMain::launch_type == DEFAULT) {
            PprzMain::launch_type = CONFIGURE;
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
            auto fp_files = GlobalState::get()->get("FLIGHTPLAN_FILES").toStringList();
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

