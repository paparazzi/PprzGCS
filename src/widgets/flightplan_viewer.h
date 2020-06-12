#ifndef FLIGHTPLAN_VIEWER_H
#define FLIGHTPLAN_VIEWER_H

#include <QWidget>
#include <QtWidgets>
#include "flowlayout.h"
#include "aircraft.h"

class FlightPlanViewer : public QWidget
{
    Q_OBJECT
public:
    explicit FlightPlanViewer(QWidget *parent = nullptr);

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *event);

private:
    std::map<int, QString> ac_ids;

    void handleNewAC(QString ac_id);

    QVBoxLayout* layout;
    QHBoxLayout* head_row;
    QComboBox* ac_combo;
    QStackedLayout* stack_layout;

};


class FPInnerViewer : public QWidget
{
    Q_OBJECT
public:
    explicit FPInnerViewer(Aircraft& ac, QWidget *parent = nullptr);

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *event);

private:

    void update_stage_display();

    Aircraft& ac;

    QVBoxLayout* layout;
    FlowLayout* blocks_layout;
    QListWidget* stages_list;

    bool block_locked;
    shared_ptr<Block> bLocked;

};

#endif // FLIGHTPLAN_VIEWER_H
