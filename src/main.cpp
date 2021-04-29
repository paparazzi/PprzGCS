#include "pprzmain.h"
#include <QApplication>
#include <iostream>
#include "layout_builder.h"
#include <QNetworkProxy>
#include <QProcessEnvironment>
#include "pprz_dispatcher.h"
#include <QWizard>
#include "configure.h"
#include "gcs_utils.h"
#include <QSettings>
#include "PprzApplication.h"

#ifndef APP_DATA_PATH
#error "you need to define APP_DATA_PATH!"
#endif


void launch_main_app() {
    QSettings settings(qApp->property("SETTINGS_PATH").toString(), QSettings::IniFormat);
    QFile file(settings.value("APP_STYLE_FILE").toString());
    if(!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "could not open " << settings.value("APP_STYLE_FILE").toString();
        exit(-1);
    }
    QTextStream stream(&file);
    qApp->setStyleSheet(stream.readAll());

    QString layout_path = settings.value("APP_LAYOUT_FILE").toString();

    PprzMain* w = build_layout(layout_path);

    PprzDispatcher::get()->start();

    w->show();
}

int main(int argc, char *argv[])
{
    int return_code = 0;
    bool setting_restarted = false;
    do {
        PprzApplication a(argc, argv);

        auto settings_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/settings.conf";
        pprzApp()->setProperty("SETTINGS_PATH", settings_path);
        QSettings settings(settings_path, QSettings::IniFormat);


        QCoreApplication::setApplicationName("PprzGCS");
        QCoreApplication::setApplicationVersion("0.1");

        QCommandLineParser parser;
        parser.setApplicationDescription("Test helper");
        parser.addHelpOption();
        parser.addVersionOption();

        QCommandLineOption configureOption("c", "Configure app settings");
        parser.addOption(configureOption);

        QCommandLineOption silentModeOption("s", "Silent mode");
        parser.addOption(silentModeOption);

        parser.process(a);


        QString config_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir conf_dir(config_path);
        if(!conf_dir.exists()) {
            conf_dir.mkpath(conf_dir.path());
        }

        settings.setValue("USER_DATA_PATH", config_path);
        settings.setValue("APP_DATA_PATH", APP_DATA_PATH);

        configure();

        a.init();

        PprzDispatcher::get()->setSilent(parser.isSet(silentModeOption));

        if(parser.isSet(configureOption) && !setting_restarted) {
            auto setedit = new SettingsEditor(true);
            setedit->show();
            setting_restarted = true;
        } else {
            launch_main_app();
        }


        return_code = qApp->exec();
        a._shutdown();
    } while(return_code == PprzMain::EXIT_CODE_REBOOT);
    return return_code;
}

