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
    QString  cfgFilePath = config_path + "/conf.txt";
    QFile conf_file(cfgFilePath);

    // take config file from command line
    if(gcsConfigPath != "" && remember) {
        QFileInfo fi(cfgFilePath);
        QDir dirName = fi.dir();
        if(!dirName.exists()) {
            dirName.mkpath(dirName.path());
        }
        if(conf_file.open(QFile::WriteOnly | QFile::Text)) {
            QFileInfo gcp(gcsConfigPath);
            QString file_content = gcp.absoluteFilePath() + "\n";
            conf_file.write(file_content.toStdString().c_str());
            conf_file.close();
        } else {
            qWarning() << "Fail to write to " << cfgFilePath;
        }
    }

    // if config file is not provided, search in persistant config
    if(gcsConfigPath == "") {
        if(!conf_file.open(QFile::ReadOnly | QFile::Text)) {
            qDebug() << "no config file! Please provide a configuration file.";
            std::cout << parser.helpText().toStdString();
            return -1;
        }

        gcsConfigPath = conf_file.readAll().trimmed();
    }

    // this should be the GCS configuration file.
    configure(gcsConfigPath);
    launch_main_app();
    return qApp->exec();

}

