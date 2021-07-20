#pragma once
#include <QString>
#include <QApplication>
#include <QFile>
#include <QSettings>

enum LogLevel {
    LOG_LOW,
    LOG_MID,
    LOG_HIGH,
};

QString user_or_app_path(QString data_path);

double parse_coordinate(QString str);

QSettings getAppSettings();

bool verbose();
void setVerbose(bool v);
bool speech();
void setSpeech(bool s);

void logDebug(QString log_class, QString msg, LogLevel level = LogLevel::LOG_MID);
