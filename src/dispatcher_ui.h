#ifndef UI_DISPATCHER_H
#define UI_DISPATCHER_H

#include <QObject>
#include "waypoint.h"
#include "point2dlatlon.h"

class Setting;

class DispatcherUi : public QObject
{
    Q_OBJECT

    static DispatcherUi* singleton;
    explicit DispatcherUi(QObject *parent = nullptr);

public:
    static DispatcherUi* get(QObject *parent = nullptr) {
        if(!singleton) {
            singleton = new DispatcherUi(parent);
        }
        return singleton;
    }

signals:
    void ac_selected(QString);
    void new_ac_config(QString);
    void move_waypoint(const shared_ptr<Waypoint>, QString ac_id);
    void settingUpdated(QString ac_id, shared_ptr<Setting>, float value);
    //void create_waypoint(Waypoint*);

public slots:

};

#endif // UI_DISPATCHER_H
