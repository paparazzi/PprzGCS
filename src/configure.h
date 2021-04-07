#ifndef CONFIGURE_H
#define CONFIGURE_H

#include <QString>
#include <QDialog>
#include <QtWidgets>

void configure();

class SettingsEditor : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsEditor(QWidget *parent = nullptr);



private:

    enum Type {
        STRING,
        INT,
        DOUBLE,
        PATH_DIR,
        PATH_FILE,
    };

    std::function<void()> addSetting(QString name, QString key, QWidget* w, QGridLayout* gl, int &row, Type type);

    QList<std::function<void()>> callbacks;

};

#endif // CONFIGURE_H
