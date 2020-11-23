#include "pprzmain.h"
#include <QApplication>
#include <iostream>
#include "layout_builder.h"
#include <QNetworkProxy>
#include <QProcessEnvironment>
#include "pprz_dispatcher.h"
#include <QWizard>
#include "configure.h"

void launch_main_app() {
    QFile file(qApp->property("APP_STYLE_FILE").toString());
    file.open(QFile::ReadOnly | QFile::Text);
    assert(file.isOpen());
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

    QCommandLineOption rememberConfigOption("r", "Remember config file.");
    parser.addOption(rememberConfigOption);

    parser.process(a);

    QString gcsConfigPath = parser.value(config_file_option);
    bool remember = parser.isSet(rememberConfigOption);

    QString config_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir conf_dir(config_path);
    if(!conf_dir.exists()) {
        conf_dir.mkpath(conf_dir.path());
    }

    QString  cfgFilePath = config_path + "/conf.txt";


    // link conf.txt to the given config file
    if(gcsConfigPath != "" && remember) {
        QFile::link(QFileInfo(gcsConfigPath).absoluteFilePath(), cfgFilePath);
    }

    // if config file is not provided, take the current link
    if(gcsConfigPath == "") {
        if(QFile(cfgFilePath).exists()) {
            gcsConfigPath = cfgFilePath;
        } else {
            qDebug() << "conf.txt not found, you must provide a config file with the -c option.";
            exit(-1);
        }
    }

    // this should be the GCS configuration file.
    configure(gcsConfigPath);
    launch_main_app();
    return qApp->exec();

}

