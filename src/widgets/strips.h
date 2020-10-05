#ifndef STRIPS_H
#define STRIPS_H

#include <QWidget>
#include <QStackedWidget>
#include "configurable.h"

class Strips : public QStackedWidget, public Configurable
{
    Q_OBJECT
public:
    explicit Strips(QWidget *parent = nullptr);
    void configure(QDomElement) {};

signals:

private:
    void handleNewAC(QString ac_id);

    std::map<QString, int> ac_ids;
public slots:
};

#endif // STRIPS_H
