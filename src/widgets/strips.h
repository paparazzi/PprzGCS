#ifndef STRIPS_H
#define STRIPS_H

#include <QWidget>
#include <QLayout>
#include "configurable.h"

class Strips : public QWidget, public Configurable
{
    Q_OBJECT
public:
    explicit Strips(QWidget *parent = nullptr);
    void configure(QDomElement) {};

signals:

private:
    void handleNewAC(QString ac_id);

    std::map<QString, QWidget*> strips;
public slots:
};

#endif // STRIPS_H
