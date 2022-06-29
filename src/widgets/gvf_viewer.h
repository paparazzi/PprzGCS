#ifndef GVF_SETTINGS_H
#define GVF_SETTINGS_H

#include <QWidget>
#include <QtWidgets>
#include <QBoxLayout>
#include <QMouseEvent>
//#include "pprz_dispatcher.h"

class GVFViewer : public QWidget
{
    Q_OBJECT
public:
    explicit GVFViewer(QString ac_id, QWidget *parent = nullptr);

private:
    void init();

    QString ac_id;
    QVBoxLayout* main_layout;
    QVBoxLayout* settings_layout;

    QPushButton* vis_button;

    bool field_vis;
    bool traj_vis;
};

#endif // GVF_SETTINGS_H