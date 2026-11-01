#include "panels.h"
#include "mapwidget.h"
#include "gcs_utils.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

////////////////////////////////// PanelManager //////////////////////////////////

PanelManager* PanelManager::get() {
    static PanelManager* instance = new PanelManager(qApp);
    return instance;
}

PanelManager::PanelManager(QObject* parent) : QObject(parent) {}

QString PanelManager::idFromMime(const QMimeData* mime) {
    if(mime == nullptr || !mime->hasFormat(MIME)) {
        return QString();
    }
    return QString::fromUtf8(mime->data(MIME));
}

void PanelManager::registerPanel(QString id, QWidget* w, QString title, QString icon, PanelTabs* tabs) {
    w->setProperty("panel_id", id);
    Panel p;
    p.widget = w;
    p.title = title;
    p.icon = icon;
    p.location = tabs ? tabs->id() : "rail";
    p.initial_location = p.location;
    p.initial_index = tabs ? tabs->indexOf(w) : 0;
    panels[id] = p;
}

void PanelManager::registerTabs(PanelTabs* tabs) {
    tab_groups.append(tabs);
}

void PanelManager::setMap(MapWidget* m) {
    if(_map == nullptr) {
        _map = m;
        registerTabs(m->bottomTabs());
    }
}

PanelTabs* PanelManager::tabsById(QString id) {
    for(auto &t: tab_groups) {
        if(t && t->id() == id) {
            return t;
        }
    }
    return nullptr;
}

/// Take the panel out of its tab group or the rail.
void PanelManager::detach(QString id) {
    auto &p = panels[id];
    if(p.location == "rail") {
        if(_map) {
            _map->removeRailPanel(p.widget);
        }
    } else if(auto tabs = tabsById(p.location)) {
        int index = tabs->indexOf(p.widget);
        if(index >= 0) {
            tabs->removeTab(index);
        }
    }
    p.location = "";
}

void PanelManager::moveToTabs(QString id, PanelTabs* tabs, int index) {
    if(!contains(id) || tabs == nullptr) {
        return;
    }
    auto &p = panels[id];
    if(p.location == tabs->id() && tabs->indexOf(p.widget) == index) {
        return;
    }
    detach(id);
    index = tabs->insertTab(index, p.widget, p.title);
    tabs->setCurrentIndex(index);
    p.widget->show();
    p.location = tabs->id();
    save();
    QTimer::singleShot(0, this, &PanelManager::cleanupEmptySplits);
}

PanelTabs* PanelManager::createSplitBelow(PanelTabs* above, QString id) {
    auto splitter = qobject_cast<QSplitter*>(above->parentWidget());
    if(splitter == nullptr) {
        return nullptr;
    }
    if(id.isEmpty()) {
        do {
            id = QString("split_%1").arg(++split_count);
        } while(tabsById(id));
    }
    auto tabs = new PanelTabs(id);
    int index = splitter->indexOf(above);
    splitter->insertWidget(index + 1, tabs);
    // the new group takes half of the one above, once the splitter has its size (at start the
    // window is not shown yet)
    QPointer<QSplitter> sp = splitter;
    QPointer<PanelTabs> new_tabs = tabs;
    QPointer<PanelTabs> above_tabs = above;
    QTimer::singleShot(0, this, [=]() {
        if(!sp || !new_tabs || !above_tabs) {
            return;
        }
        auto sizes = sp->sizes();
        int a = sp->indexOf(above_tabs), b = sp->indexOf(new_tabs);
        if(a < 0 || b < 0) {
            return;
        }
        int total = sizes[a] + sizes[b];
        sizes[a] = total - total / 2;
        sizes[b] = total / 2;
        sp->setSizes(sizes);
    });
    tabs->setProperty("split_after", above->id());
    registerTabs(tabs);
    splits.append(tabs);
    return tabs;
}

void PanelManager::moveToNewTabsBelow(QString id, PanelTabs* above) {
    if(!contains(id) || above == nullptr) {
        return;
    }
    auto tabs = createSplitBelow(above);
    if(tabs) {
        moveToTabs(id, tabs);
    }
}

void PanelManager::moveToRail(QString id) {
    if(!contains(id) || _map == nullptr || panels[id].location == "rail") {
        return;
    }
    detach(id);
    auto &p = panels[id];
    _map->addRailPanel(id, p.widget, p.title, p.icon);
    p.location = "rail";
    save();
    QTimer::singleShot(0, this, &PanelManager::cleanupEmptySplits);
}

void PanelManager::cleanupEmptySplits() {
    for(auto &t: QList<QPointer<PanelTabs>>(splits)) {
        if(t && t->count() == 0) {
            splits.removeAll(t);
            tab_groups.removeAll(t);
            t->deleteLater();
        }
    }
    splits.removeAll(nullptr);
    tab_groups.removeAll(nullptr);
    save();
    emit panelsMoved();
}

void PanelManager::save() {
    if(restoring) {
        return;
    }
    QJsonArray js_splits;
    for(auto &t: splits) {
        if(t && t->count() > 0) {
            js_splits.append(QJsonObject{{"id", t->id()}, {"after", t->property("split_after").toString()}});
        }
    }
    QJsonObject js_panels;
    for(auto it = panels.begin(); it != panels.end(); ++it) {
        if(!it->widget) {
            continue;
        }
        int index = 0;
        if(auto tabs = tabsById(it->location)) {
            index = tabs->indexOf(it->widget);
        }
        js_panels[it.key()] = QJsonObject{{"in", it->location}, {"index", index}};
    }
    QJsonObject root{{"splits", js_splits}, {"panels", js_panels}};
    QSettings settings(appConfig()->value("SETTINGS_PATH").toString(), QSettings::IniFormat);
    settings.setValue("panels/" + layout_name, QString(QJsonDocument(root).toJson(QJsonDocument::Compact)));
}

/// Put the panels where they were last time (unknown panels or groups are skipped).
void PanelManager::restore() {
    QSettings settings(appConfig()->value("SETTINGS_PATH").toString(), QSettings::IniFormat);
    auto txt = settings.value("panels/" + layout_name).toString();
    if(txt.isEmpty()) {
        return;
    }
    auto root = QJsonDocument::fromJson(txt.toUtf8()).object();
    restoring = true;
    for(auto v: root["splits"].toArray()) {
        auto o = v.toObject();
        auto above = tabsById(o["after"].toString());
        if(above && !tabsById(o["id"].toString())) {
            createSplitBelow(above, o["id"].toString());
        }
    }
    // in the saved order, so the indexes are right
    struct Item {QString id; QString in; int index;};
    QList<Item> items;
    auto js_panels = root["panels"].toObject();
    for(auto it = js_panels.begin(); it != js_panels.end(); ++it) {
        auto o = it->toObject();
        items.append({it.key(), o["in"].toString(), o["index"].toInt()});
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {return a.index < b.index;});
    for(auto &item: items) {
        if(!contains(item.id) || panels[item.id].location == item.in) {
            continue;
        }
        if(item.in == "rail") {
            moveToRail(item.id);
        } else if(auto tabs = tabsById(item.in)) {
            moveToTabs(item.id, tabs, item.index);
        }
    }
    // the first tab of each group
    for(auto &t: tab_groups) {
        if(t && t->count() > 0) {
            t->setCurrentIndex(0);
        }
    }
    restoring = false;
    cleanupEmptySplits();
}

/// Back to the layout file's arrangement.
void PanelManager::reset() {
    restoring = true;
    QList<QString> ids = panels.keys();
    std::sort(ids.begin(), ids.end(), [this](const QString& a, const QString& b) {
        return panels[a].initial_index < panels[b].initial_index;
    });
    for(auto &id: ids) {
        auto &p = panels[id];
        if(!p.widget || p.location == p.initial_location) {
            continue;
        }
        if(p.initial_location == "rail") {
            moveToRail(id);
        } else if(auto tabs = tabsById(p.initial_location)) {
            moveToTabs(id, tabs, p.initial_index);
        }
    }
    for(auto &t: tab_groups) {
        if(t && t->count() > 0) {
            t->setCurrentIndex(0);
        }
    }
    restoring = false;
    cleanupEmptySplits();
}

/// Where a panel dragged over global_pos would go.
PanelManager::Target PanelManager::targetAt(QPoint global_pos, QString id) {
    Target t;
    auto w = QApplication::widgetAt(global_pos);
    for(; w != nullptr; w = w->parentWidget()) {
        if(w == drop_hint) {
            continue;
        }
        if(w->objectName() == "map_rail") {
            t.kind = RAIL;
            t.rect = QRect(w->mapToGlobal(QPoint(0, 0)), w->size());
            return t;
        }
        if(auto tabs = qobject_cast<PanelTabs*>(w)) {
            QPoint pos = tabs->mapFromGlobal(global_pos);
            QRect r(tabs->mapToGlobal(QPoint(0, 0)), tabs->size());
            bool alone = panels.value(id).location == tabs->id() && tabs->count() == 1;
            bool on_bar = tabs->tabBar()->geometry().contains(pos);
            if(!on_bar && tabs->isSplittable() && pos.y() > tabs->height() * 0.6 && !alone) {
                t.kind = SPLIT;
                t.rect = QRect(r.left(), r.top() + r.height() / 2, r.width(), r.height() - r.height() / 2);
            } else {
                t.kind = tabs->id() == "map_bottom" ? BOTTOM : TABS;
                t.rect = r;
                if(on_bar) {
                    t.index = tabs->tabBar()->tabAt(tabs->tabBar()->mapFrom(tabs, pos));
                }
            }
            t.tabs = tabs;
            return t;
        }
        if(auto map = qobject_cast<MapWidget*>(w)) {
            t.kind = BOTTOM;
            t.tabs = map->bottomTabs();
            int h = map->height() * 35 / 100;
            QRect r(map->mapToGlobal(QPoint(0, map->height() - h)), QSize(map->width(), h));
            t.rect = r;
            return t;
        }
    }
    return t;
}

void PanelManager::startDrag(QString id, QWidget* source, QPixmap pixmap) {
    if(!contains(id) || !dragged_id.isEmpty()) {
        return;
    }
    auto window = source->window();
    dragged_id = id;
    drop_hint = new QWidget(window);
    drop_hint->setObjectName("panel_drop_hint");
    drop_hint->setAttribute(Qt::WA_TransparentForMouseEvents);
    drop_hint->setStyleSheet("#panel_drop_hint {background: rgba(29, 95, 201, 50); border: 2px solid #1d5fc9; border-radius: 6px;}");
    drop_hint->hide();

    bool accepted_drops = window->acceptDrops();
    window->setAcceptDrops(true);
    qApp->installEventFilter(this);

    auto drag = new QDrag(source);
    auto mime = new QMimeData();
    mime->setData(MIME, id.toUtf8());
    drag->setMimeData(mime);
    if(!pixmap.isNull()) {
        drag->setPixmap(pixmap.scaledToWidth(qMin(pixmap.width(), 160), Qt::SmoothTransformation));
    }
    drag->exec(Qt::MoveAction);

    qApp->removeEventFilter(this);
    window->setAcceptDrops(accepted_drops);
    if(drop_hint) {
        drop_hint->deleteLater();
    }
    dragged_id.clear();
}

bool PanelManager::eventFilter(QObject* obj, QEvent* event) {
    (void)obj;
    auto type = event->type();
    if(type != QEvent::DragEnter && type != QEvent::DragMove && type != QEvent::Drop && type != QEvent::DragLeave) {
        return false;
    }
    if(type == QEvent::DragLeave) {
        if(drop_hint) {
            drop_hint->hide();
        }
        return false;
    }
    auto e = static_cast<QDropEvent*>(event);
    QString id = idFromMime(e->mimeData());
    if(id.isEmpty()) {
        return false;
    }
    auto receiver = qobject_cast<QWidget*>(obj);
    if(receiver == nullptr) {
        return false;
    }
    QPoint global = receiver->mapToGlobal(e->position().toPoint());
    auto t = targetAt(global, id);

    if(type == QEvent::Drop) {
        if(drop_hint) {
            drop_hint->hide();
        }
        // after the drag (the source tab bar or rail button may be removed)
        QPointer<PanelTabs> tabs = t.tabs;
        int index = t.index;
        switch(t.kind) {
        case RAIL:
            QTimer::singleShot(0, this, [=,this]() {moveToRail(id);});
            break;
        case TABS:
        case BOTTOM:
            QTimer::singleShot(0, this, [=,this]() {if(tabs) moveToTabs(id, tabs, index);});
            break;
        case SPLIT:
            QTimer::singleShot(0, this, [=,this]() {if(tabs) moveToNewTabsBelow(id, tabs);});
            break;
        case NONE:
            e->ignore();
            return true;
        }
        e->acceptProposedAction();
        return true;
    }

    if(t.kind == NONE) {
        if(drop_hint) {
            drop_hint->hide();
        }
        e->ignore();
        return true;
    }
    if(drop_hint) {
        auto window = drop_hint->parentWidget();
        drop_hint->setGeometry(QRect(window->mapFromGlobal(t.rect.topLeft()), t.rect.size()));
        drop_hint->raise();
        drop_hint->show();
    }
    e->acceptProposedAction();
    return true;
}

////////////////////////////////// PanelTabs //////////////////////////////////

PanelTabs::PanelTabs(QString id, QWidget* parent, bool splittable) : QTabWidget(parent),
    _id(id), splittable(splittable)
{
    setObjectName(id);
    setTabBar(new PanelTabBar(this));
    setDocumentMode(true);
    hide();     // until a panel is added
}

int PanelTabs::indexOfPanel(QString id) {
    for(int i=0; i<count(); i++) {
        if(panelAt(i) == id) {
            return i;
        }
    }
    return -1;
}

QString PanelTabs::panelAt(int index) {
    auto w = widget(index);
    if(w == nullptr) {
        return QString();
    }
    return w->property("panel_id").toString();
}

void PanelTabs::tabInserted(int index) {
    QTabWidget::tabInserted(index);
    setVisible(count() > 0);
}

void PanelTabs::tabRemoved(int index) {
    QTabWidget::tabRemoved(index);
    setVisible(count() > 0);
}

////////////////////////////////// PanelTabBar //////////////////////////////////

PanelTabBar::PanelTabBar(PanelTabs* tabs) : QTabBar(tabs), tabs(tabs) {
    setToolTip("Drag a tab to move it: to another group, below a group, to the bottom of the map or to the rail");
}

void PanelTabBar::mousePressEvent(QMouseEvent* e) {
    press_pos = e->position().toPoint();
    press_index = tabAt(press_pos);
    QTabBar::mousePressEvent(e);
}

void PanelTabBar::mouseMoveEvent(QMouseEvent* e) {
    if((e->buttons() & Qt::LeftButton) && press_index >= 0 &&
       (e->position().toPoint() - press_pos).manhattanLength() > QApplication::startDragDistance() * 2) {
        QString id = tabs->panelAt(press_index);
        auto pix = grab(tabRect(press_index));
        press_index = -1;
        PanelManager::get()->startDrag(id, this, pix);
        return;
    }
    QTabBar::mouseMoveEvent(e);
}

void PanelManager::showPanel(QString id) {
    if(!contains(id)) {
        return;
    }
    auto &p = panels[id];
    if(p.location == "rail") {
        if(_map) {
            _map->openRailPanel(p.widget);
        }
    } else if(auto tabs = tabsById(p.location)) {
        tabs->setCurrentWidget(p.widget);
    }
}
