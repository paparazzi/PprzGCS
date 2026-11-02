#include "map_notifications.h"
#include "AircraftManager.h"

static const int MAX_ROWS = 3;
static const int MESSAGE_MS = 6000;
static const int BUSY_MESSAGE_MS = 3000;
static const int MERGE_MS = 4000;

MapNotifications::MapNotifications(QWidget *parent) : QWidget(parent)
{
    setObjectName("map_notifications");
    setAttribute(Qt::WA_TranslucentBackground);
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);
    more_button = new QPushButton(this);
    more_button->setObjectName("notification_more");
    more_button->setCursor(Qt::PointingHandCursor);
    connect(more_button, &QPushButton::clicked, this, &MapNotifications::openChat);
    timer = new QTimer(this);
    timer->setInterval(250);
    connect(timer, &QTimer::timeout, this, &MapNotifications::expire);
    timer->start();
    if(parent) {
        parent->installEventFilter(this);
    }
    hide();
}

QString MapNotifications::senderName(QString sender, QColor* color) {
    *color = QColor(0x6e7781);
    if(sender == "me") {
        *color = QColor(0x1d5fc9);
        return "Me";
    }
    if(AircraftManager::get()->aircraftExists(sender)) {
        auto ac = AircraftManager::get()->getAircraft(sender);
        *color = ac->getColor();
        return ac->name();
    }
    if(sender.isEmpty() || sender == "ground") {
        return "Server";
    }
    return sender.left(1).toUpper() + sender.mid(1);
}

void MapNotifications::message(QString sender, QString text) {
    auto now = QDateTime::currentDateTime();
    recent.append(now);
    while(!recent.isEmpty() && recent.first().msecsTo(now) > 5000) {
        recent.removeFirst();
    }
    int duration = recent.size() > MAX_ROWS ? BUSY_MESSAGE_MS : MESSAGE_MS;

    // a repeat of the same sender: update that row
    for(auto &it: items) {
        if(it.level == INFO && it.key == "msg:" + sender && it.time.msecsTo(now.time()) < MERGE_MS) {
            it.text = text;
            it.count += 1;
            it.time = now.time();
            it.expires = now.addMSecs(duration);
            rebuild();
            return;
        }
    }
    Item it;
    it.key = "msg:" + sender;
    it.sender = sender;
    it.text = text;
    it.level = INFO;
    it.time = now.time();
    it.expires = now.addMSecs(duration);
    items.append(it);
    rebuild();
}

void MapNotifications::setState(QString key, QString sender, QString text, Level level) {
    for(int i=0; i<items.size(); i++) {
        auto &it = items[i];
        if(it.key != key) {
            continue;
        }
        if(level == INFO) {
            items.removeAt(i);
        } else {
            // a dismissed one comes back when it gets worse
            if(it.dismissed && level > it.dismissed_level) {
                it.dismissed = false;
            }
            if(it.text != text || it.level != level) {
                it.time = QTime::currentTime();
            }
            it.text = text;
            it.level = level;
        }
        rebuild();
        return;
    }
    if(level == INFO) {
        return;
    }
    Item it;
    it.key = key;
    it.sender = sender;
    it.text = text;
    it.level = level;
    it.time = QTime::currentTime();
    items.append(it);
    rebuild();
}

void MapNotifications::expire() {
    auto now = QDateTime::currentDateTime();
    bool changed = false;
    for(int i=items.size()-1; i>=0; i--) {
        auto &it = items[i];
        if(it.expires.isValid() && !it.hovered && it.expires < now) {
            items.removeAt(i);
            changed = true;
        }
    }
    if(changed) {
        rebuild();
    }
}

QWidget* MapNotifications::makeRow(const Item& item) {
    auto row = new QFrame(this);
    static const char* names[] = {"notification_info", "notification_warning", "notification_alarm"};
    row->setObjectName(names[item.level]);
    row->setAttribute(Qt::WA_StyledBackground);
    row->setCursor(Qt::PointingHandCursor);
    row->setProperty("key", item.key);
    row->installEventFilter(this);
    auto h = new QHBoxLayout(row);
    h->setContentsMargins(10, 3, 4, 3);
    h->setSpacing(8);

    QColor color;
    QString name = senderName(item.sender, &color);
    auto tag = new QLabel(row);
    tag->setObjectName("notification_tag");
    QFontMetrics fm(tag->font());
    tag->setText(fm.elidedText(name, Qt::ElideRight, 96));
    tag->setToolTip(name);
    tag->setAlignment(Qt::AlignCenter);
    tag->setMinimumWidth(56);
    tag->setStyleSheet(QString("background: %1; color: white; border-radius: 4px; font-size: 11px; font-weight: bold; padding: 1px 6px;").arg(color.name()));
    h->addWidget(tag);

    auto text = new QLabel(item.text, row);
    text->setObjectName("notification_text");
    text->setWordWrap(true);
    h->addWidget(text, 1);
    if(item.count > 1) {
        auto count = new QLabel(QString::fromUtf8("\xC3\x97") + QString::number(item.count), row);
        count->setObjectName("notification_count");
        h->addWidget(count);
    }
    auto time = new QLabel(item.time.toString("HH:mm"), row);
    time->setObjectName("notification_time");
    time->setToolTip(item.time.toString("HH:mm:ss"));
    h->addWidget(time);
    if(item.level != INFO) {
        auto close = new QToolButton(row);
        close->setObjectName("notification_close");
        close->setText(QString::fromUtf8("\xE2\x9C\x95"));
        close->setToolTip("Dismiss (comes back when it gets worse)");
        QString key = item.key;
        connect(close, &QToolButton::clicked, this, [=,this]() {
            for(auto &it: items) {
                if(it.key == key) {
                    it.dismissed = true;
                    it.dismissed_level = it.level;
                }
            }
            rebuild();
        });
        h->addWidget(close);
    } else {
        h->addSpacing(6);
    }
    return row;
}

void MapNotifications::rebuild() {
    while(layout->count() > 0) {
        auto it = layout->takeAt(0);
        if(it->widget() && it->widget() != more_button) {
            it->widget()->deleteLater();
        }
        delete it;
    }
    // alarms, then warnings, then messages (newest first)
    QList<const Item*> shown;
    for(int level = ALARM; level >= INFO; level--) {
        for(int i=items.size()-1; i>=0; i--) {
            if(items[i].level == level && !items[i].dismissed) {
                shown.append(&items[i]);
            }
        }
    }
    int extra = qMax(0, static_cast<int>(shown.size()) - MAX_ROWS);
    for(int i=0; i<qMin(MAX_ROWS, static_cast<int>(shown.size())); i++) {
        auto row = makeRow(*shown[i]);
        layout->addWidget(row);
        row->show();     // created while the stack may be hidden
    }
    if(extra > 0) {
        more_button->setText(QString("+%1 more in the chat").arg(extra));
        layout->addWidget(more_button, 0, Qt::AlignHCenter);
        more_button->show();
    } else {
        more_button->hide();
    }
    setVisible(!shown.isEmpty());
    reposition();
}

void MapNotifications::avoid(QWidget* panel) {
    panel->installEventFilter(this);
    reposition();
}

/// Centered in the part of the map right of the open panels (rail cards), so they stay visible.
void MapNotifications::reposition() {
    auto p = parentWidget();
    if(!p) {
        return;
    }
    // in the widest free space at the top, between the cards that reach the top of the map
    // (by the rail, or moved there)
    QList<QPair<int, int>> blocked;
    for(auto card: p->findChildren<QFrame*>("rail_card")) {
        if(card->isVisible()) {
            QRect r(card->mapTo(p, QPoint(0, 0)), card->size());
            if(r.top() < 150) {
                blocked.append({r.left() - 10, r.right() + 10});
            }
        }
    }
    std::sort(blocked.begin(), blocked.end());
    int best_left = 60, best_right = 60;
    int pos = 60;     // after the rail
    int right_edge = p->width() - 10;
    blocked.append({right_edge, right_edge});
    for(auto &b: blocked) {
        int end = qMin(b.first, right_edge);
        if(end - pos > best_right - best_left) {
            best_left = pos;
            best_right = end;
        }
        pos = qMax(pos, b.second);
    }
    int left = best_left;
    int right = best_right;
    // as wide as the messages ask (one line), at most 680, within the free space
    int wanted = 420;
    for(auto label: findChildren<QLabel*>()) {
        if(label->wordWrap() && label->isVisible()) {
            QFontMetrics fm(label->font());
            int needed = fm.horizontalAdvance(label->text()) + 300;   // sender, count, time, close
            wanted = qMax(wanted, needed);
        }
    }
    int w = qBound(200, qMin(wanted, 680), qMax(200, right - left));
    int x = left + qMax(0, (right - left - w) / 2);
    setFixedWidth(w);
    layout->activate();
    int h = layout->hasHeightForWidth() ? layout->heightForWidth(w) : layout->sizeHint().height();
    setFixedHeight(qMax(h, layout->minimumSize().height()));
    move(x, 10);
    raise();
}

bool MapNotifications::eventFilter(QObject* obj, QEvent* event) {
    if(obj == parentWidget() && event->type() == QEvent::Resize) {
        reposition();
        return false;
    }
    if(obj != parentWidget() && obj->objectName() == "rail_card" &&
       (event->type() == QEvent::Show || event->type() == QEvent::Hide || event->type() == QEvent::Resize ||
        event->type() == QEvent::Move)) {
        QTimer::singleShot(0, this, &MapNotifications::reposition);    // after the layout placed it
        return false;
    }
    auto row = qobject_cast<QFrame*>(obj);
    if(row && row->property("key").isValid()) {
        QString key = row->property("key").toString();
        if(event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
            // a message stays while the pointer is on it
            for(auto &it: items) {
                if(it.key == key) {
                    it.hovered = event->type() == QEvent::Enter;
                    if(!it.hovered && it.expires.isValid()) {
                        it.expires = qMax(it.expires, QDateTime::currentDateTime().addMSecs(2000));
                    }
                }
            }
        } else if(event->type() == QEvent::MouseButtonRelease) {
            for(auto &it: items) {
                if(it.key == key) {
                    emit senderClicked(it.sender);
                }
            }
            return true;
        }
    }
    return false;
}
