#include "chat.h"
#include "ui_chat.h"
#include "pprz_dispatcher.h"
#include "gcs_utils.h"
#include "AircraftManager.h"
#include "dispatcher_ui.h"
#include "chatbubble.h"
#include <QtWidgets>
#include <QDebug>
#include <ostream>

Chat::Chat(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::Chat)
{
    ui->setupUi(this);
    ui->label->hide();  // the title is on the card or the tab
    ui->send_button->setObjectName("chat_send");
    ui->send_button->setText("Send");
    ui->msg_lineedit->setPlaceholderText("Message to All");

    // filter row at the top
    filter_row = new QWidget(this);
    filter_layout = new QHBoxLayout(filter_row);
    filter_layout->setContentsMargins(0, 0, 0, 0);
    filter_layout->setSpacing(4);
    filter_group = new QButtonGroup(this);
    filter_group->setExclusive(true);
    addFilter("all", "All", "");
    addFilter("server", "Server", "");
    addFilter("me", "Mine", "");
    filter_layout->addStretch();
    filters["all"]->setChecked(true);
    filter_row->hide();
    // rounded pills; set here so the card styles do not override them
    filter_row->setStyleSheet(
        "QPushButton#chat_filter {border: 1px solid palette(mid); border-radius: 10px; padding: 2px 10px;"
        " font-size: 12px; font-weight: bold; background: transparent;}"
        "QPushButton#chat_filter:checked {background: #1d5fc9; border-color: #1d5fc9; color: white;}");
    // stay at the newest message when new ones arrive (unless scrolled up)
    auto bar = ui->scrollArea->verticalScrollBar();
    connect(bar, &QScrollBar::rangeChanged, this, [=](int min, int max) {
        (void)min;
        if(bar->property("at_end").toBool() || !bar->property("at_end").isValid()) {
            bar->setValue(max);
        }
    });
    connect(bar, &QScrollBar::valueChanged, this, [=](int v) {
        bar->setProperty("at_end", v >= bar->maximum() - 4);
    });
    ui->verticalLayout->insertWidget(1, filter_row);

    // one input row: message, recipient and log menu, send
    ui->name_combo->hide();
    ui->fd_combo->hide();
    dest_button = new QToolButton(this);
    dest_button->setObjectName("chat_dest");
    dest_button->setPopupMode(QToolButton::InstantPopup);
    dest_button->setToolTip("To, and where it is logged on the aircraft");
    auto menu = new QMenu(dest_button);
    dest_button->setMenu(menu);
    connect(menu, &QMenu::aboutToShow, this, [=,this]() {
        menu->clear();
        menu->addSection("To");
        for(int i=0; i<ui->name_combo->count(); i++) {
            auto a = menu->addAction(ui->name_combo->itemText(i));
            a->setCheckable(true);
            a->setChecked(i == ui->name_combo->currentIndex());
            connect(a, &QAction::triggered, this, [=,this]() {ui->name_combo->setCurrentIndex(i); updateDestButton();});
        }
        menu->addSection("Log in");
        for(int i=0; i<ui->fd_combo->count(); i++) {
            auto a = menu->addAction(ui->fd_combo->itemText(i));
            a->setCheckable(true);
            a->setChecked(i == ui->fd_combo->currentIndex());
            connect(a, &QAction::triggered, this, [=,this]() {ui->fd_combo->setCurrentIndex(i); updateDestButton();});
        }
    });
    auto input_row = new QHBoxLayout();
    input_row->setSpacing(6);
    ui->verticalLayout->removeWidget(ui->msg_lineedit);
    ui->horizontalLayout->removeWidget(ui->send_button);
    input_row->addWidget(ui->msg_lineedit, 1);
    input_row->addWidget(dest_button);
    input_row->addWidget(ui->send_button);
    ui->verticalLayout->addLayout(input_row);
    updateDestButton();
    setFrameShape(QFrame::NoFrame);
    setMinimumWidth(320);
    ui->scrollArea->setMinimumHeight(300);     // a few messages visible
    ui->scrollArea->setFrameShape(QFrame::NoFrame);
    ui->verticalLayout->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout->setSpacing(4);
    ui->scroll_widget->layout()->setContentsMargins(0, 0, 4, 0);
    ui->scroll_widget->layout()->setSpacing(3);

    auto settings_path = appConfig()->value("SETTINGS_PATH").toString();
    QSettings settings(settings_path, QSettings::IniFormat);
    chat_id = settings.value("pprzlink/id").toString();

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this,  &Chat::onNewAircraft);
    connect(DispatcherUi::get(), &DispatcherUi::ac_deleted, this, &Chat::onAcDeleted);

    PprzDispatcher::get()->bind("INFO_MSG", this, [=,this](QString sender, pprzlink::Message msg) {
        QByteArray ar;
        msg.getField("msg", ar);
        auto txt = QString(ar);
        addMessage(txt, sender);
    });

    try {
        PprzDispatcher::get()->bind("INFO_MSG_GROUND", this, [=,this](QString sender, pprzlink::Message msg) {
            (void) sender;
            QString txt;
            QString source;
            QString dest;
            msg.getField("msg", txt);
            msg.getField("source", source);
            msg.getField("dest", dest);
            auto dest_splitted = dest.split(':');
            QString chat_dst;

            if(dest_splitted[0] == chat_id) {
                chat_dst = chat_id;
            } else {
                bool isInt;
                int dest_id = dest_splitted[0].toInt(&isInt, 0);
                if(isInt) {
                    if(dest_id == 0xFF) {
                        chat_dst = "All";
                    } else if(dest_id == 0) {
                        chat_dst = "Ground";
                    }
                }
            }
            if(!chat_dst.isNull()) {
                addMessage(txt, source, chat_dst);
            }
        });

        connect(ui->msg_lineedit, &QLineEdit::returnPressed, this, &Chat::onSend);
        connect(ui->send_button, &QPushButton::clicked, this, &Chat::onSend);

    } catch (pprzlink::no_such_message &e) {
        qDebug() << e.what();
        ui->msg_lineedit->setDisabled(true);
        ui->send_button->setDisabled(true);
        ui->fd_combo->setDisabled(true);
        ui->name_combo->setDisabled(true);
        ui->msg_lineedit->setPlaceholderText("Update paparazzi to send messages");
    }

    // Notice for an aircraft reboot (PprzDispatcher reloads the aircraft)
    PprzDispatcher::get()->bind("AIRCRAFT_REBOOT", this, [=,this](QString sender, pprzlink::Message msg) {
        (void)sender;
        QString id, reason;
        msg.getField("ac_id", id);
        msg.getField("reason", reason);
        if(!AircraftManager::get()->aircraftExists(id)) {
            return;
        }
        auto ac = AircraftManager::get()->getAircraft(id);
        QString txt = "Rebooted (" + reason + "): waypoints";
        txt += ac->getChecklist().isEmpty() ? " reset" : " and checklist reset";
        addMessage(txt, ac->name());
    });

    setAutoFillBackground(true);
}

Chat::~Chat()
{
    delete ui;
}

void Chat::onNewAircraft(QString ac_id) {
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    ui->name_combo->addItem(ac->name());
    addFilter(ac_id, ac->name(), ac->getColor().name());
}

void Chat::addFilter(QString category, QString name, QString color) {
    if(filters.contains(category)) {
        return;
    }
    auto b = new QPushButton(name, filter_row);
    b->setObjectName("chat_filter");
    b->setCheckable(true);
    if(!color.isEmpty()) {
        b->setStyleSheet(QString("QPushButton#chat_filter:!checked {color: %1;}").arg(color));
    }
    filter_group->addButton(b);
    // aircraft before Server and Mine
    int index = filter_layout->count();
    if(category != "all" && category != "server" && category != "me" && filters.contains("server")) {
        index = filter_layout->indexOf(filters["server"]);
    }
    filter_layout->insertWidget(index, b);
    filters[category] = b;
    connect(b, &QPushButton::clicked, this, [=,this]() {
        current_filter = category;
        applyFilter();
    });
    int nb_ac = 0;
    for(auto &k: filters.keys()) {
        if(k != "all" && k != "server" && k != "me") {
            nb_ac++;
        }
    }
    filter_row->setVisible(nb_ac > 1);
}

void Chat::applyFilter() {
    for(auto cb: ui->scroll_widget->findChildren<ChatBubble*>()) {
        cb->setVisible(current_filter == "all" || cb->category() == current_filter);
    }
}

void Chat::updateDestButton() {
    dest_button->setText(ui->name_combo->currentText() + QString::fromUtf8(" \u25BE"));
    ui->msg_lineedit->setPlaceholderText("Message to " + ui->name_combo->currentText());
}

void Chat::onAcDeleted(QString ac_id) {
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    auto idx = ui->name_combo->findText(ac->name());
    if(idx != -1) {
        ui->name_combo->removeItem(idx);
    }
    if(filters.contains(ac_id)) {
        auto b = filters.take(ac_id);
        if(current_filter == ac_id) {
            current_filter = "all";
            filters["all"]->setChecked(true);
        }
        b->deleteLater();
        applyFilter();
        int nb_ac = filters.size() - 3;
        filter_row->setVisible(nb_ac > 1);
    }
}

void Chat::onSend() {
    auto text = ui->msg_lineedit->text();
    auto fd = ui->fd_combo->currentIndex();
    auto name = ui->name_combo->currentText();
    QString dest;

    if(name == "All") {
        dest = "0xFF";
    } else {
        dest = AircraftManager::get()->getAircraftByName(name).value()->getId();
    }
    dest += ":" + QString::number(fd);

    auto msgDef = PprzDispatcher::get()->getDict()->getDefinition("INFO_MSG_GROUND");
    pprzlink::Message pprz_msg(msgDef);
    pprz_msg.addField("dest", dest);
    pprz_msg.addField("source", chat_id);
    pprz_msg.addField("msg", text);
    PprzDispatcher::get()->sendMessage(pprz_msg);
    addMessage(text, "me", name, true);

    ui->msg_lineedit->clear();
}


void Chat::addMessage(QString txt, QString source, QString dst, bool sent) {
    (void)dst;
    // Check if it can be combined with the previous message
    if(ui->scroll_widget->layout()->count() > 0) {
        auto last = ui->scroll_widget->layout()->itemAt(ui->scroll_widget->layout()->count()-1);
        auto cb = dynamic_cast<ChatBubble*>(last->widget());
        if(cb != nullptr && cb->getSource() == source && cb->getDest() == dst && cb->getTime().secsTo(QTime::currentTime()) < 5 &&
           cb->currentLevel() == ChatBubble::level(txt)){   // a warning gets its own row
            cb->addText(txt);
            return;
        }
    }

    auto cb = new ChatBubble(source, dst, txt, sent, this);
    ui->scroll_widget->layout()->addWidget(cb);
    cb->setVisible(current_filter == "all" || cb->category() == current_filter);

    // delay scroll to bottom to wait for the x=widget to be repainted.
    QTimer::singleShot(20, this, [=,this](){
        ui->scrollArea->verticalScrollBar()->setSliderPosition(ui->scrollArea->verticalScrollBar()->maximum());
    });

}
