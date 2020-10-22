#ifndef STRIPS_H
#define STRIPS_H

#include <QWidget>
#include <QLayout>
#include "configurable.h"
#include "strip.h"

class Strips : public QWidget, public Configurable
{
    Q_OBJECT
public:
    explicit Strips(QWidget *parent = nullptr);
    void configure(QDomElement) {};

signals:

private:
    void handleNewAC(QString ac_id);

    std::map<QString, Strip*> strips;
    QVBoxLayout* scroll_layout;
public slots:
};

#endif // STRIPS_H
