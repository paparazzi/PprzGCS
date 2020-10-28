#include "pprzmain.h"
#include <QApplication>
#include <iostream>
#include "layout_builder.h"
#include <QNetworkProxy>
#include <QProcessEnvironment>
#include "pprz_dispatcher.h"
#include <QWizard>

void configure(QFile& config_file) {
    assert(config_file.isOpen());
    QTextStream stream(&config_file);

    if(!qEnvironmentVariableIsSet("PAPARAZZI_HOME") ||
       !qEnvironmentVariableIsSet("PAPARAZZI_SRC")) {
        std::cerr << "Set environnements variables PAPARAZZI_HOME, PAPARAZZI_SRC!" << std::endl;
        abort();
    }

    QString PAPARAZZI_HOME = qgetenv("PAPARAZZI_HOME");
    QString PAPARAZZI_SRC = qgetenv("PAPARAZZI_SRC");
    QString PAPARAZZI_GCS_DATA = qgetenv("PAPARAZZI_GCS_DATA");

    qApp->setProperty("IVY_NAME", "QPprzControl");
    qApp->setProperty("IVY_BUS", "127.255.255.255:2010");
    qApp->setProperty("PPRZLINK_ID", "pprzcontrol");

    qApp->setProperty("PPRZLINK_MESSAGES", PAPARAZZI_HOME + "/var/messages.xml");
    qApp->setProperty("PPRZLINK_MESSAGES", PAPARAZZI_HOME + "/var/messages.xml");
    qApp->setProperty("PATH_GCS_ICON", PAPARAZZI_HOME + "/data/pictures/gcs_icons");
    qApp->setProperty("DEFAULT_TILE_PROVIDER", "Google");

    qApp->setProperty("APP_DATA_PATH", PAPARAZZI_GCS_DATA);
    qApp->setProperty("MAP_MOVE_HYSTERESIS", 20);
    qApp->setProperty("WAYPOINTS_SIZE", 8);
    qApp->setProperty("CIRCLE_CREATE_MIN_RADIUS", 1.0);
    qApp->setProperty("CIRCLE_STROKE", 4);
    qApp->setProperty("SIZE_HIGHLIGHT_FACTOR", 1.5);

    qApp->setProperty("ITEM_Z_VALUE_HIGHLIGHTED", 100);
    qApp->setProperty("ITEM_Z_VALUE_UNHIGHLIGHTED", 50);
    qApp->setProperty("NAV_SHAPE_Z_VALUE", 150);
    qApp->setProperty("AIRCRAFT_Z_VALUE", 300);

    qApp->setProperty("MAPITEMS_FONT", 18);
    qApp->setProperty("AIRCRAFTS_SIZE", 40);

    qApp->setProperty("DEFAULT_COLOR", "red");
    qApp->setProperty("PATH_AIRCRAFT_ICON", PAPARAZZI_GCS_DATA + "/pictures/aircraft_icons");

    qApp->setProperty("TRACK_MAX_CHUNKS", 10);
    qApp->setProperty("TRACK_CHUNCK_SIZE", 20);


    qApp->setProperty("APP_STYLE_FILE", PAPARAZZI_GCS_DATA + "/conf/style.qss");
    qApp->setProperty("APP_LAYOUT_FILE", PAPARAZZI_GCS_DATA + "/conf/default_layout.xml");

}

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

    QString config_path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    qDebug() << config_path;

    QString  cfgFilePath = config_path + "/conf.txt";

    QFile conf_file(cfgFilePath);
    if(conf_file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "file exists!";
        configure(conf_file);
        launch_main_app();
        return qApp->exec();
    } else {
        qDebug() << "no config file!";
        auto wizard = new QWizard();
        auto page1 = new QWizardPage(wizard);
        page1->setTitle("App path");
        page1->setSubTitle("PprzCGS need some configuration and data files. Configure it here!");
        page1->setPixmap(QWizard::LogoPixmap, QPixmap(":/pictures/icon.svg"));
        //page1->set
        wizard->addPage(page1);

        QObject::connect(wizard->button(QWizard::FinishButton), &QAbstractButton::clicked, [=]{
            qDebug() << "wizard finised!";

            // dummy config file creation, just to be able to start the application
            QFile config_file(cfgFilePath);
            config_file.open(QFile::WriteOnly | QFile::Text);
            config_file.close();

            QFile conf_file(cfgFilePath);
            if(conf_file.open(QFile::ReadOnly | QFile::Text)) {
                configure(conf_file);
                launch_main_app();
            } else {
                qDebug() << "PprzGCS has not been configured ! No configuration file at " << cfgFilePath << "!";
            }
        });

        wizard->show();

        return qApp->exec();
    }

}

