#ifndef PANELS_H
#define PANELS_H

#include <QtWidgets>
#include <QPointer>
#include <functional>

class PanelTabs;
class MapWidget;

/**
 * Panels (flight plan, settings, checklist, chat, PFD, ...) can be dragged between tab groups
 * (<tabs> in the layout), a new tab group under another one (split), the bottom of the map and
 * the rail of buttons on the map. The arrangement is saved per layout file and restored at start.
 */
class PanelManager : public QObject
{
    Q_OBJECT
public:
    static PanelManager* get();
    static constexpr const char* MIME = "application/x-pprzgcs-panel";

    void setLayoutName(QString name) {layout_name = name;}

    /// A panel in a tab group, or (tabs == nullptr) in the map rail
    void registerPanel(QString id, QWidget* w, QString title, QString icon, PanelTabs* tabs);
    void registerTabs(PanelTabs* tabs);
    void setMap(MapWidget* m);
    MapWidget* map() {return _map;}

    bool contains(QString id) {return panels.contains(id) && panels[id].widget;}
    QString title(QString id) {return panels.value(id).title;}
    QString icon(QString id) {return panels.value(id).icon;}

    /// Start dragging a panel (from a tab or a rail button)
    void startDrag(QString id, QWidget* source, QPixmap pixmap);

    void moveToTabs(QString id, PanelTabs* tabs, int index = -1);
    void moveToNewTabsBelow(QString id, PanelTabs* above);
    void moveToRail(QString id);
    /// Show a panel: open it on the rail or select its tab
    void showPanel(QString id);

    void restore();
    void reset();

    static QString idFromMime(const QMimeData* mime);

signals:
    void panelsMoved();     // e.g. to fit the strips again

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    explicit PanelManager(QObject* parent = nullptr);
    enum TargetKind {NONE, RAIL, TABS, SPLIT, BOTTOM};
    struct Target {TargetKind kind = NONE; PanelTabs* tabs = nullptr; int index = -1; QRect rect;};
    Target targetAt(QPoint global_pos, QString id);
    void detach(QString id);
    void save();
    void cleanupEmptySplits();
    PanelTabs* tabsById(QString id);
    PanelTabs* createSplitBelow(PanelTabs* above, QString id = QString());

    struct Panel {
        QPointer<QWidget> widget;
        QString title;
        QString icon;
        QString location;           // tab group id, or "rail"
        QString initial_location;
        int initial_index = 0;
    };
    QMap<QString, Panel> panels;
    QList<QPointer<PanelTabs>> tab_groups;
    QList<QPointer<PanelTabs>> splits;      // created by a split
    MapWidget* _map = nullptr;
    QString layout_name = "default";
    bool restoring = false;
    int split_count = 0;
    QPointer<QWidget> drop_hint;
    QString dragged_id;
};

/// A tab group that panels can be dragged out of and dropped on (drops are handled by the
/// PanelManager, also when the cursor is over the content of a panel).
class PanelTabs : public QTabWidget
{
    Q_OBJECT
public:
    explicit PanelTabs(QString id, QWidget* parent = nullptr, bool splittable = true);
    QString id() {return _id;}
    bool isSplittable() {return splittable;}
    int indexOfPanel(QString id);
    QString panelAt(int index);

protected:
    void tabInserted(int index) override;
    void tabRemoved(int index) override;

private:
    QString _id;
    bool splittable;
};

/// The tab bar of a PanelTabs: dragging a tab moves its panel.
class PanelTabBar : public QTabBar
{
    Q_OBJECT
public:
    explicit PanelTabBar(PanelTabs* tabs);
protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
private:
    PanelTabs* tabs;
    QPoint press_pos;
    int press_index = -1;
};


#endif // PANELS_H
