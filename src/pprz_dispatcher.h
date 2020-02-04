#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <QObject>

class PprzMessage {};

class PprzDispatcher : public QObject
{
    Q_OBJECT

    static PprzDispatcher* singleton;
    explicit PprzDispatcher(QObject *parent = nullptr);

public:
    static PprzDispatcher* get(QObject *parent = nullptr) {
        if(!singleton) {
            singleton = new PprzDispatcher(parent);
        }
        return singleton;
    }

signals:
    void gps(PprzMessage);
    void alive(PprzMessage);
    void takeoff(PprzMessage);
    void attitude(PprzMessage);
    void pprz_mode(PprzMessage);

public slots:

};

#endif // DISPATCHER_H
