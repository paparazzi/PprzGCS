#ifndef WATCHER_H
#define WATCHER_H

#include <QObject>
#include "PprzToolbox.h"
#include "PprzApplication.h"
#include "pprz_dispatcher.h"

class Watcher : public PprzTool
{
    Q_OBJECT
public:
    explicit Watcher(PprzApplication* app, PprzToolbox* toolbox);
    void setToolbox(PprzToolbox* toolbox) override;

public:
    enum BatStatus{
        OK,
        LOW,
        CRITIC,
        CATASTROPHIC,
    };

signals:
    void bat_status(QString ac_id, BatStatus);

private:
    void bat_watcher(pprzlink::Message msg);

    // bat alarm
    struct BatParams {
        float low;
        float critic;
        float catastrophic;
    };
    QMap<QString, BatParams> ac_bat_params;


};

#endif // WATCHER_H
