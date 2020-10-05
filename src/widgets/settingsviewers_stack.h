#ifndef SETTINGSVIEWERS_STACK_H
#define SETTINGSVIEWERS_STACK_H

#include <QStackedWidget>
#include "settings_viewer.h"
#include "configurable.h"
#include "ac_selector.h"

class SettingsViewersStack : public QWidget, public Configurable
{
    Q_OBJECT
public:
    explicit SettingsViewersStack(QWidget *parent = nullptr);
    void configure(QDomElement) {};

signals:

public slots:

private:
    void handleNewAC(QString ac_id);

    QMap<QString, int> viewers_indexes;
    QList<QPushButton*> buttons;

    QVBoxLayout* vLayout;
    ACSelector* ac_selector;
    QStackedWidget* stack;

};

#endif // SETTINGSVIEWERS_STACK_H
