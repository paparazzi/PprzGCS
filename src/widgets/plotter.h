#ifndef PLOTTER_H
#define PLOTTER_H

#include <QtWidgets>
#include "graphwidget.h"
#include "pprz_dispatcher.h"
#include "configurable.h"

class Plotter : public QWidget, public Configurable
{
    Q_OBJECT
public:
    explicit Plotter(QString ac_id, QWidget *parent = nullptr);

    void configure(QDomElement c);
    void addGraph(QString name, GraphWidget::Params p);

    void changeGraph(QString name);

signals:

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
    virtual void dragMoveEvent(QDragMoveEvent *event) override;

private:
    struct DataId {
        QString msg_class;
        QString msg_name;
        QString field;
    };

private slots:
    void onOpenContextMenu();
    void handleMsg(QString name, QString sender, pprzlink::Message msg);

private:

    template<typename T>
    void feedGraph(GraphWidget* graph, QString field, pprzlink::Message msg);

    QLabel* title;
    QStackedWidget* graph_stack;
    QMap<QString, GraphWidget*> graphs;
    QMap<QString, long> bids;
    QString current_name;

    QToolButton* var_button;
    QCheckBox* autoscale_checkbox;

    QString ac_id;
};

#endif // PLOTTER_H
