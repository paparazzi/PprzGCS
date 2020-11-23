#include "gcs_utils.h"
#include <QDebug>

QString user_or_app_path(QString data_path) {
    auto path = qApp->property("USER_DATA_PATH").toString() + data_path;
    if(QFile::exists(path)) {
        return path;
    } else {
        return qApp->property("APP_DATA_PATH").toString() + data_path;
    }
}
