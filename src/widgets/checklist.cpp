#include "card_dialog.h"
#include "checklist.h"
#include <QScrollArea>
#include "ui_checklist.h"
#include "pprz_dispatcher.h"
#include "gcs_utils.h"
#include "AircraftManager.h"
#include "dispatcher_ui.h"
#include <QtWidgets>
#include <memory>
#include <QDebug>
#include <ostream>

namespace {
/// A checklist item: a 24 px box (easy to hit on a touch screen), the whole row can be tapped,
/// a line above it; done items in grey.
class ChecklistCheck : public QCheckBox {
public:
    explicit ChecklistCheck(const QString& text, QWidget* parent = nullptr) : QCheckBox(text, parent) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);
    }
    QSize sizeHint() const override {
        return QSize(BOX + GAP + fontMetrics().horizontalAdvance(text()) + 8, 40);
    }
    QSize minimumSizeHint() const override { return QSize(BOX + GAP + 40, 40); }

protected:
    bool hitButton(const QPoint& pos) const override { return rect().contains(pos); }
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QColor text_color = palette().color(QPalette::WindowText);
        QColor line = text_color;
        line.setAlphaF(0.10f);
        p.setPen(QPen(line, 1));
        p.drawLine(0, 0, width(), 0);
        QRectF box(2.5, (height() - BOX) / 2.0 + 0.5, BOX - 1, BOX - 1);
        if(isChecked()) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0x1f7a3f));
            p.drawRoundedRect(box, 5, 5);
            p.setPen(QPen(Qt::white, 2.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            QPointF o = box.topLeft();
            p.drawPolyline(QPolygonF({o + QPointF(6, 12.5), o + QPointF(10, 16.5), o + QPointF(17.5, 7.5)}));
        } else {
            QColor border = text_color;
            border.setAlphaF(0.45f);
            p.setPen(QPen(hasFocus() ? palette().color(QPalette::Highlight) : border, 2));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(box.adjusted(0.5, 0.5, -0.5, -0.5), 5, 5);
        }
        QColor c = text_color;
        if(isChecked()) {
            c.setAlphaF(0.5f);     // done: grey
        }
        p.setPen(c);
        QRect tr = rect().adjusted(BOX + GAP, 0, 0, 0);
        p.drawText(tr, Qt::AlignVCenter | Qt::AlignLeft,
                   fontMetrics().elidedText(text(), Qt::ElideRight, tr.width()));
    }

private:
    static constexpr int BOX = 24;
    static constexpr int GAP = 10;
};
}

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

    // these colors when the application style sheet does not style them
    if(!qApp->styleSheet().contains("checklist_progress")) {
        setStyleSheet(
            "QProgressBar#checklist_progress {border: 0; border-radius: 4px; background: #e3e7ec;}"
            "QProgressBar#checklist_progress::chunk {border-radius: 4px; background: #1f7a3f;}"
            "QLabel#checklist_count {font-family: \"Monospace\";}");
    }
    auto markers = std::make_shared<QMap<ChecklistItem*, QWidget*>>();  // the items counted in the progress

    auto refresh_progress = [=,this]() {
        int total = 0, done = 0;
        for(auto item: ac->getChecklist()) {
            if(item->optional || !markers->contains(item)) {
                continue;
            }
            total += 1;
            bool is_done = item->type == "checkbox" ? item->value == "true" : !item->value.isEmpty();
            done += is_done ? 1 : 0;
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
        row->setContentsMargins(0, 3, 0, 3);
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
            auto widget_item = new ChecklistCheck(item->description);
            widget_item->setToolTip(item->help.isEmpty() ? item->description : item->help);
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
        if(CardDialog::ask(this, "Reset the checklist of " + ac->name() + "?",
                           "All items go back to unchecked for the next flight. Remembered values stay filled in.",
                           "Reset checklist", CardDialog::DANGER)) {
            reset_checklist();
        }
    });

    refresh_progress();

    // the items scroll below the header (title, progress, Reset) when there is not enough height,
    // instead of being squeezed
    auto items = new QWidget();
    items->setObjectName("checklist_items");
    auto items_lay = new QVBoxLayout(items);
    items_lay->setContentsMargins(0, 0, 0, 0);
    items_lay->setSpacing(0);     // the checks are rows with a line between them
    while(ui->verticalLayout->count() > 1) {
        auto item = ui->verticalLayout->takeAt(1);
        if(item->widget()) {
            items_lay->addWidget(item->widget());
            delete item;
        } else if(item->layout()) {
            items_lay->addLayout(item->layout());   // label and field rows (their widgets move along)
        } else {
            items_lay->addItem(item);
        }
    }
    items_lay->addStretch(1);    // items at the top when there is extra height (e.g. in a tab)
    auto scroll = new QScrollArea(this);
    scroll->setWidget(items);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->viewport()->setAutoFillBackground(false);
    items->setAutoFillBackground(false);
    ui->verticalLayout->addWidget(scroll, 1);
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
