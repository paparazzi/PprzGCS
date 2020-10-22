#ifndef WIDGET_STACK_H
#define WIDGET_STACK_H

#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include "configurable.h"
#include "ac_selector.h"

class WidgetStack : public QWidget, public Configurable
{
    Q_OBJECT
public:
    explicit WidgetStack(std::function<QWidget*(QString, QWidget*)> constructor, QWidget *parent = nullptr);
    void configure(QDomElement) {};

private:
    void handleNewAC(QString ac_id);

    QMap<QString, QWidget*> viewers_widgets;

    QVBoxLayout* vLayout;
    ACSelector* ac_selector;
    QVBoxLayout* stackLayout;

    std::function<QWidget*(QString, QWidget*)> constructor;
};

#endif // WIDGET_STACK_H
