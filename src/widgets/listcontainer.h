#ifndef LISTCONTAINER_H
#define LISTCONTAINER_H

#include <QWidget>
#include <QLayout>
#include "configurable.h"
#include "strip.h"

class ListContainer : public QWidget, public Configurable
{
    Q_OBJECT
public:
    explicit ListContainer(std::function<QWidget*(QString, QWidget*)> constructor, QWidget *parent = nullptr);
    void configure(QDomElement) {};

signals:

private:
    void handleNewAC(QString ac_id);
    void removeAC(QString ac_id);

    std::function<QWidget*(QString, QWidget*)> constructor;

    QMap<QString, QWidget*> widgets;
    QVBoxLayout* scroll_layout;
    QScrollArea* scroll;
public slots:
};

#endif // LISTCONTAINER_H
