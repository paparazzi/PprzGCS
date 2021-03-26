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

#ifndef APP_DATA_PATH
#error "you need to define APP_DATA_PATH!"
#endif


void launch_main_app() {
    QFile file(qApp->property("APP_STYLE_FILE").toString());
    if(!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "could not open " << qApp->property("APP_STYLE_FILE").toString();
        exit(-1);
    }
    QTextStream stream(&file);
    qApp->setStyleSheet(stream.readAll());

    QString layout_path = qApp->property("APP_LAYOUT_FILE").toString();

    QMainWindow* w = build_layout(layout_path);

    PprzDispatcher::get()->start();

    w->show();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName("PprzGCS");
    QApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Test helper");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption config_file_option(QStringList() << "c" << "config",
               "Start application with the given config file.",
               "config_file");
    parser.addOption(config_file_option);

    QCommandLineOption silentModeOption("s", "Silent mode");
    parser.addOption(silentModeOption);

    parser.process(a);

    QString arg_config_path = parser.value(config_file_option);

    QString config_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir conf_dir(config_path);
    if(!conf_dir.exists()) {
        conf_dir.mkpath(conf_dir.path());
    }
    qApp->setProperty("USER_DATA_PATH", config_path);
    qApp->setProperty("APP_DATA_PATH", APP_DATA_PATH);


    //default config path
    auto gcsConfigPath = user_or_app_path("conf.txt");
    if(arg_config_path != "") {
        //user provided a conf file
        gcsConfigPath = arg_config_path;
    }

    // this should be the GCS configuration file.
    configure(gcsConfigPath);

    PprzDispatcher::get()->setSilent(parser.isSet(silentModeOption));

    launch_main_app();
    return qApp->exec();
}

