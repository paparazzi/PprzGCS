#include "checklist.h"
#include "ui_checklist.h"
#include "pprz_dispatcher.h"
#include "gcs_utils.h"
#include "AircraftManager.h"
#include "dispatcher_ui.h"
#include "chatbubble.h"
#include <QtWidgets>
#include <memory>
#include <QDebug>
#include <ostream>

Checklist::Checklist(QString ac_id, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::Checklist)
{
    // Get the global settings
    auto settings_path = appConfig()->value("SETTINGS_PATH").toString();
    QSettings settings(settings_path, QSettings::IniFormat);
    pprzlink_id = settings.value("pprzlink/id").toString();

    // Setup the UI
    ui->setupUi(this);

    // Get the uav settings and find the PFC settings
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    auto ac_settings = ac->getSettingMenu()->getAllSettings();
    Setting* pfc_setting = nullptr;
    for(auto setting: ac_settings) {
        if(setting->getFullName() == "preflight_ground_done") {
            pfc_setting = setting;
        }
    }

    // Title row: title, progress (bar and count of the required items) and the reset button
    ui->verticalLayout->removeWidget(ui->label);
    auto header = new QHBoxLayout();
    header->setSpacing(10);
    header->addWidget(ui->label);
    auto progress = new QProgressBar(this);
    progress->setObjectName("checklist_progress");
    progress->setTextVisible(false);
    progress->setFixedHeight(8);
    header->addWidget(progress, 1);
    auto count_label = new QLabel(this);
    count_label->setObjectName("checklist_count");
    header->addWidget(count_label);
    auto reset_button = new QPushButton("Reset");
    reset_button->setToolTip("Clear the checklist for the next flight");
    header->addWidget(reset_button);
    ui->verticalLayout->insertLayout(0, header);
    ui->verticalLayout->setSpacing(8);

    // Required items that are not done are shown in amber; these colors when the application
    // style sheet does not style them
    if(!qApp->styleSheet().contains("checklist_open")) {
        setStyleSheet(
            "*[checklist_open=\"true\"] {color: #8a4b00; font-weight: bold;}"
            "QProgressBar#checklist_progress {border: 0; border-radius: 4px; background: #e3e7ec;}"
            "QProgressBar#checklist_progress::chunk {border-radius: 4px; background: #1f7a3f;}"
            "QLabel#checklist_count {font-family: \"Monospace\";}");
    }
    auto markers = std::make_shared<QMap<ChecklistItem*, QWidget*>>();  // the widget that shows an item is open

    auto refresh_progress = [=,this]() {
        int total = 0, done = 0;
        for(auto item: ac->getChecklist()) {
            if(item->optional || !markers->contains(item)) {
                continue;
            }
            total += 1;
            bool is_done = item->type == "checkbox" ? item->value == "true" : !item->value.isEmpty();
            done += is_done ? 1 : 0;
            auto w = (*markers)[item];
            if(w->property("checklist_open").toBool() != !is_done) {
                w->setProperty("checklist_open", !is_done);
                w->style()->unpolish(w);
                w->style()->polish(w);
            }
        }
        progress->setMaximum(qMax(total, 1));
        progress->setValue(done);
        count_label->setText(QString("%1/%2").arg(done).arg(total));
    };

    QString remember_prefix = "checklist/" + ac->name() + "/";

    // Send the values not logged yet (e.g. pre-filled ones) once the checklist is complete,
    // and update the preflight setting
    auto update_finished = [=,this]() {
        bool finished = ac->checklistFinished();
        if(finished) {
            for(auto item: ac->getChecklist()) {
                if(!item->logged && item->value != "") {
                    sendMessage(ac_id, item);
                }
            }
        }
        if(pfc_setting != nullptr) {
            ac->setSetting(pfc_setting, finished? 1:0);
        }
    };

    // A value entered by the user: log it, remember it and update the checklist state
    auto set_value = [=,this](ChecklistItem* item, QString value) {
        item->value = value;
        sendMessage(ac_id, item);
        if(item->remember) {
            QSettings settings(settings_path, QSettings::IniFormat);
            settings.setValue(remember_prefix + item->name, value);
        }
        update_finished();
        refresh_progress();
    };

    // The value an item starts with (after loading or a reset)
    auto initial_value = [=](ChecklistItem* item) {
        if(item->remember && item->type != "checkbox") {
            QSettings settings(settings_path, QSettings::IniFormat);
            return settings.value(remember_prefix + item->name).toString();
        }
        return QString();
    };

    // Label and input on one row, with the help text as tooltip
    auto add_row = [=,this](ChecklistItem* item, QWidget* input) {
        auto row = new QHBoxLayout();
        auto label = new QLabel(item->description);
        label->setToolTip(item->help);
        label->setMinimumWidth(110);
        input->setToolTip(item->help);
        input->setMinimumHeight(30);
        row->addWidget(label);
        row->addWidget(input, 1);
        ui->verticalLayout->addLayout(row);
        (*markers)[item] = label;
    };

    // Go through the checklist items and create the widget
    QList<std::function<void()>> resets;
    QList<ChecklistItem*> checklist = ac->getChecklist();
    for(auto item: checklist) {
        if(item->type != "checkbox" && item->value.isEmpty()) {
            item->value = initial_value(item);
        }

        if(item->type == "checkbox") {
            auto widget_item = new QCheckBox(item->description);
            widget_item->setToolTip(item->help);
            widget_item->setMinimumHeight(30);
            (*markers)[item] = widget_item;
            if(item->value == "true") {
                widget_item->setChecked(true);
            }
            ui->verticalLayout->addWidget(widget_item);

            connect(widget_item, &QCheckBox::toggled, this,
            [=](bool state) {
                set_value(item, state ? "true" : "false");
            });
            resets.append([=]() {
                QSignalBlocker blocker(widget_item);
                widget_item->setChecked(false);
            });
        }
        else if(item->type == "text") {
            auto widget_input = new QLineEdit(item->value);
            if(item->optional) {
                widget_input->setPlaceholderText("optional");
            }
            add_row(item, widget_input);

            connect(widget_input, &QLineEdit::editingFinished, this,
            [=]() {
                if(widget_input->text() != item->value || !item->logged) {
                    set_value(item, widget_input->text());
                }
            });
            resets.append([=]() {
                QSignalBlocker blocker(widget_input);
                widget_input->setText(item->value);
            });
        }
        else if(item->type == "choice") {
            auto widget_input = new QComboBox();
            widget_input->addItems(item->values);
            widget_input->setPlaceholderText(item->optional ? "optional" : "select");
            widget_input->setCurrentIndex(item->values.indexOf(item->value));
            add_row(item, widget_input);

            connect(widget_input, &QComboBox::activated, this,
            [=]() {
                set_value(item, widget_input->currentText());
            });
            resets.append([=]() {
                QSignalBlocker blocker(widget_input);
                widget_input->setCurrentIndex(item->values.indexOf(item->value));
            });
        }
    }

    // Clear the checklist; remembered values stay filled in, and are logged again on completion
    auto reset_checklist = [=]() {
        for(auto item: ac->getChecklist()) {
            item->value = (item->type == "checkbox") ? QString() : initial_value(item);
            item->logged = false;
        }
        for(auto &reset: resets) {
            reset();
        }
        refresh_progress();
        if(pfc_setting != nullptr) {
            ac->setSetting(pfc_setting, 0);
        }
    };

    connect(reset_button, &QPushButton::clicked, this, [=,this]() {
        auto answer = QMessageBox::question(this, "Reset checklist",
            "Clear the checklist of " + ac->name() + " for the next flight?");
        if(answer == QMessageBox::Yes) {
            reset_checklist();
        }
    });

    refresh_progress();
    ui->verticalLayout->addStretch(1);    // items at the top when there is extra height (e.g. in a tab)
    setAutoFillBackground(true);
}

void Checklist::sendMessage(QString ac_id, ChecklistItem *item) {
    auto msgDef = PprzDispatcher::get()->getDict()->getDefinition("INFO_MSG_GROUND");
    pprzlink::Message pprz_msg(msgDef);
    pprz_msg.addField("dest", ac_id);
    pprz_msg.addField("source", pprzlink_id);
    pprz_msg.addField("msg", "[PFC] " + item->name + ": " + item->value);
    PprzDispatcher::get()->sendMessage(pprz_msg);
    item->logged = true;
}

Checklist::~Checklist()
{
    delete ui;
}
