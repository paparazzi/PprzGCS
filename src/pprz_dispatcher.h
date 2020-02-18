#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <QObject>
#include <pprzlink/IvyLink.h>
#include <memory>

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
    void gps(pprzlink::Message);
    void alive(pprzlink::Message);
    void takeoff(pprzlink::Message);
    void attitude(pprzlink::Message);
    void pprz_mode(pprzlink::Message);

public slots:

private:
    std::unique_ptr<pprzlink::MessageDictionary> dict;
    std::unique_ptr<pprzlink::IvyLink> link;

};

#endif // DISPATCHER_H
