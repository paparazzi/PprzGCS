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

    QMap<QString, int> viewers_indexes;

    QVBoxLayout* vLayout;
    ACSelector* ac_selector;
    QStackedWidget* stack;

    std::function<QWidget*(QString, QWidget*)> constructor;
};

#endif // WIDGET_STACK_H
