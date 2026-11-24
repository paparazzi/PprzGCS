#pragma once
#include <QString>
#include <QApplication>
#include <QFile>
#include <QSettings>
#include "globalconfig.h"
#include <pprzlinkQt/Message.h>

enum LogLevel {
    LOG_LOW,
    LOG_MID,
    LOG_HIGH,
};

QString user_or_app_path(QString data_path);

double parse_coordinate(QString str);

double getFloatingField(pprzlink::Message &msg, const QString &field);

QSettings getAppSettings();
GlobalConfig* appConfig();

bool verbose();
void setVerbose(bool v);
bool speech();

/// The device profiles (e.g. "cyberzoo", "field"): the microphones, cameras and alert output of a place,
/// set up on each computer (Settings, Recording). "" is the default profile (the keys without a prefix).
/// The profiles of this computer, without the default one
QStringList profileNames();
/// The profile of this run: pprzgcs --profile NAME when this computer has it, else the default ("")
QString activeProfile();
/// The profile asked with --profile (also when this computer does not have it)
QString requestedProfile();
/// The key of a setting in a profile (e.g. "video/cameras")
QString profileKey(QString key, QString profile);
void setSpeech(bool s);

void logDebug(QString log_class, QString msg, LogLevel level = LogLevel::LOG_MID);
QStringList getLayoutFiles(QString location);
QStringList getXmlFiles(QString location, QString rootTag);
