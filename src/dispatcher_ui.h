#ifndef UI_DISPATCHER_H
#define UI_DISPATCHER_H

#include <QObject>

class Waypoint {};  // dummy class just for test

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
    void ac_selected(int);
    void create_waypoint(Waypoint*);

public slots:

};

#endif // UI_DISPATCHER_H
