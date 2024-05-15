#include "checklist.h"
#include "ui_checklist.h"
#include "pprz_dispatcher.h"
#include "gcs_utils.h"
#include "AircraftManager.h"
#include "dispatcher_ui.h"
#include "chatbubble.h"
#include <QtWidgets>
#include <QDebug>
#include <ostream>

Checklist::Checklist(QString ac_id, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::Checklist)
{
    // Get the settings
    auto settings_path = appConfig()->value("SETTINGS_PATH").toString();
    QSettings settings(settings_path, QSettings::IniFormat);
    pprzlink_id = settings.value("pprzlink/id").toString();

    // Setup the UI
    ui->setupUi(this);

    QList<ChecklistItem*> checklist = AircraftManager::get()->getAircraft(ac_id)->getChecklist();
    for(auto item: checklist) {
        if(item->type == "checkbox") {
            auto widget_item = new QCheckBox(item->description);
            if(item->value == "true") {
                widget_item->setChecked(true);
            }
            ui->verticalLayout->addWidget(widget_item);

            connect(widget_item, &QCheckBox::toggled, this, 
            [=](bool state) {
                item->value = (state)? QString("true") : QString("false");
                sendMessage(ac_id, item);
            });
        }
        else if(item->type == "text") {
            auto widget_item = new QHBoxLayout();
            auto widget_label = new QLabel(item->description);
            auto widget_input = new QLineEdit();
            widget_item->addWidget(widget_label);
            widget_item->addWidget(widget_input);
            ui->verticalLayout->addLayout(widget_item);

            connect(widget_input, &QLineEdit::returnPressed, this, 
            [=]() {
                item->value = widget_input->text();
                sendMessage(ac_id, item);
            });
        }
    }

    setAutoFillBackground(true);
}

void Checklist::sendMessage(QString ac_id, ChecklistItem *item) {
    auto msgDef = PprzDispatcher::get()->getDict()->getDefinition("INFO_MSG_GROUND");
    pprzlink::Message pprz_msg(msgDef);
    pprz_msg.addField("dest", ac_id);
    pprz_msg.addField("source", pprzlink_id);
    pprz_msg.addField("msg", "[PFC] " + item->name + ": " + item->value);
    PprzDispatcher::get()->sendMessage(pprz_msg);
}

Checklist::~Checklist()
{
    delete ui;
}
