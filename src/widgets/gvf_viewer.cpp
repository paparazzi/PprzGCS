#include "gvf_viewer.h"
#include "dispatcher_ui.h"

// So far this widget looks soo basic but we'll add more GVF setting/info here in a next future :)

GVFViewer::GVFViewer(QString ac_id, QWidget *parent) : QWidget(parent), ac_id(ac_id)
{
    main_layout = new QVBoxLayout(this);
    settings_layout = new QVBoxLayout();
    auto hspacer = new QSpacerItem(10,1);

    auto vis_layout = new QHBoxLayout();
    auto vis_label= new QLabel("Vector Field", this);
    vis_button = new QPushButton(this);
    vis_button->setText(QString("ON"));
    vis_layout->addWidget(vis_label);
    vis_layout->addItem(hspacer);
    vis_layout->addWidget(vis_button);

    settings_layout->addItem(vis_layout);
    main_layout->addItem(settings_layout);

    init();
}

void GVFViewer::init()
{   
    traj_vis = true;
    field_vis = true;
    emit DispatcherUi::get()->gvf_settingUpdated(ac_id, traj_vis, field_vis);

    connect(
        vis_button, &QPushButton::clicked, this,
        [=]() {
            if (vis_button->text() == "ON") {
                emit DispatcherUi::get()->gvf_settingUpdated(ac_id, traj_vis, false);
                vis_button->setText(QString("OFF"));
            } else {
                emit DispatcherUi::get()->gvf_settingUpdated(ac_id, traj_vis, true);
                vis_button->setText(QString("ON"));
            }
        }
    );
}