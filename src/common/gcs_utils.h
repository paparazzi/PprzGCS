#pragma once
#include <QString>
#include <QApplication>
#include <QFile>
#include <QSettings>

QString user_or_app_path(QString data_path);

double parse_coordinate(QString str);

QSettings getAppSettings();
