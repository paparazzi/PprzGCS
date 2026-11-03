#include "chat.h"
#include <QTimer>
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
    filter_layout = new FlowLayout(filter_row, 0, 4, 4);
    filter_group = new QButtonGroup(this);
    filter_group->setExclusive(true);
    addFilter("all", "All", "");
    addFilter("server", "Server", "");
    addFilter("me", "Mine", "");
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
    setMinimumSize(320, 0);     // (the .ui file has a 300 px minimum height: the chat fits its messages)
    ui->scrollArea->setMinimumHeight(120);     // a few messages visible; more fit (fitMessages)
    ui->scrollArea->viewport()->installEventFilter(this);   // width changes: wrap again
    ui->scrollArea->setFrameShape(QFrame::NoFrame);
    ui->verticalLayout->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout->setSpacing(4);
    ui->scroll_widget->layout()->setContentsMargins(0, 0, 4, 0);
    ui->scroll_widget->layout()->setSpacing(3);
    // the spacer above the messages (they stay at the bottom) takes no height of its own
    for(int i=0; i<ui->scroll_widget->layout()->count(); i++) {
        if(auto sp = ui->scroll_widget->layout()->itemAt(i)->spacerItem()) {
            sp->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
        }
    }

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
    PprzDispatcher::get()->bindOptional("AIRCRAFT_REBOOT", this, [=,this](QString sender, pprzlink::Message msg) {
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

void Chat::showEvent(QShowEvent *event) {
    QFrame::showEvent(event);
    QTimer::singleShot(0, this, &Chat::fitMessages);
    if(unread != 0) {
        unread = 0;
        emit unreadChanged(unread);
    }
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
    int index = filter_order.size();
    if(category != "all" && category != "server" && category != "me" && filters.contains("server")) {
        index = filter_order.indexOf(filters["server"]);
    }
    filter_order.insert(index, b);
    while(filter_layout->count() > 0) {
        delete filter_layout->takeAt(0);
    }
    for(auto fb: filter_order) {
        filter_layout->addWidget(fb);
    }
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
        filter_order.removeAll(b);
        filter_layout->removeWidget(b);
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
    if(!sent && !isVisible()) {
        unread += 1;
        emit unreadChanged(unread);
    }

    // Check if it can be combined with the previous message
    if(ui->scroll_widget->layout()->count() > 0) {
        auto last = ui->scroll_widget->layout()->itemAt(ui->scroll_widget->layout()->count()-1);
        auto cb = dynamic_cast<ChatBubble*>(last->widget());
        if(cb != nullptr && cb->getSource() == source && cb->getDest() == dst){   // the name only again when the sender changes
            cb->addText(txt);
            QTimer::singleShot(0, this, &Chat::fitMessages);
            return;
        }
    }

    auto cb = new ChatBubble(source, dst, txt, sent, this);
    ui->scroll_widget->layout()->addWidget(cb);
    QTimer::singleShot(0, this, &Chat::fitMessages);
    cb->setVisible(current_filter == "all" || cb->category() == current_filter);

    // delay scroll to bottom to wait for the x=widget to be repainted.
    QTimer::singleShot(20, this, [=,this](){
        ui->scrollArea->verticalScrollBar()->setSliderPosition(ui->scrollArea->verticalScrollBar()->maximum());
    });

}

bool Chat::eventFilter(QObject* obj, QEvent* event) {
    if((obj == ui->scrollArea->viewport() || obj->objectName() == "rail_card") && event->type() == QEvent::Resize) {
        QTimer::singleShot(0, this, &Chat::fitMessages);
    }
    return QFrame::eventFilter(obj, event);
}

void Chat::fitMessages() {
    // a scroll area does not use heightForWidth: with word wrapped messages the list was shorter
    // than its messages, the last lines hidden and out of reach of the scroll bar
    auto lay = ui->scroll_widget->layout();
    int w = ui->scrollArea->viewport()->width();
    int h = lay->hasHeightForWidth() ? lay->totalHeightForWidth(w) : lay->totalSizeHint().height();
    ui->scroll_widget->setMinimumHeight(h);

    // the chat as tall as its messages (120 to 400 px), unless the user gave its card a size
    bool user_sized = false;
    for(auto p = parentWidget(); p != nullptr; p = p->parentWidget()) {
        if(p->objectName() == "rail_card") {
            user_sized = p->property("user_size").toSize().isValid();
            if(!p->property("chat_watched").toBool()) {
                p->setProperty("chat_watched", true);
                p->installEventFilter(this);    // resized with its grip: fill it
            }
            break;
        }
    }
    int wanted = user_sized ? 120 : qBound(120, h, 400);
    // not resized by the user: exactly as tall as the messages (a scroll area asks for more)
    int max = user_sized ? QWIDGETSIZE_MAX : wanted;
    if(ui->scrollArea->minimumHeight() != wanted || ui->scrollArea->maximumHeight() != max) {
        ui->scrollArea->setMinimumHeight(wanted);
        ui->scrollArea->setMaximumHeight(max);
        updateGeometry();
    }
    // at the newest message
    auto bar = ui->scrollArea->verticalScrollBar();
    if(bar->property("at_end").toBool() || !bar->property("at_end").isValid()) {
        bar->setValue(bar->maximum());
    }
}
