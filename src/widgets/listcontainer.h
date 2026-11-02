#ifndef LISTCONTAINER_H
#define LISTCONTAINER_H

#include <QWidget>
#include <QLayout>
#include "configurable.h"
#include "strip.h"

class ListContainer : public QScrollArea, public Configurable
{
    Q_OBJECT
public:
    explicit ListContainer(std::function<QWidget*(QString, QWidget*)> constructor,std::function<QWidget*(QString, QWidget*)> alt_constructor, QWidget *parent = nullptr);
    explicit ListContainer(std::function<QWidget*(QString, QWidget*)> constructor, QWidget *parent = nullptr);
    void configure(QDomElement c) override {conf = c;}

signals:

protected:
    // a click on a widget selects its aircraft, a double click also centers the map on it
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void showSelected();
    void handleNewAC(QString ac_id);
    void removeAC(QString ac_id);
    void fitInSplitter();
    /// with more than 3 aircraft, the ones not selected are compact (when the widget can be)
    void updateCompact();
    QString selected_ac;

    std::function<QWidget*(QString, QWidget*)> constructor;
    std::function<QWidget*(QString, QWidget*)> alt_constructor;

    QMap<QString, QWidget*> widgets;
    QMap<QString, QWidget*> alt_widgets;

    QMap<QWidget*, QString> buttons;
    QGridLayout* grid_layout;
    int fitted_height = -1;     // content height when last fitted in the splitter

    QDomElement conf;

public slots:
};

#endif // LISTCONTAINER_H
