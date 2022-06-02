#ifndef AIRCRAFTWATCHER_H
#define AIRCRAFTWATCHER_H

#include <QObject>
#include "pprz_dispatcher.h"

class AircraftWatcher : public QObject
{
    Q_OBJECT
public:
    explicit AircraftWatcher(QString ac_id, QObject *parent = nullptr);
    void init();

    void watch_bat(pprzlink::Message msg);

    enum BatStatus{
        OK,
        LOW,
        CRITIC,
        CATASTROPHIC,
    };

signals:
    void bat_status(BatStatus);

private:


    QString ac_id;

    float bat_low;
    float bat_critic;
    float bat_catastrophic;


};

#endif // AIRCRAFTWATCHER_H
