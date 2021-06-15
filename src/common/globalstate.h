#ifndef GLOBALSTATE_H
#define GLOBALSTATE_H

#include <QMap>
#include <QVariant>

class GlobalState
{
public:
    static GlobalState* get() {
        if(singleton == nullptr) {
            singleton = new GlobalState();
        }
        return singleton;
    }

    bool exists(QString key);
    QVariant get(QString key);
    void set(QString key, QVariant value);

private:
    GlobalState(){};
    static GlobalState* singleton;

    QMap<QString, QVariant> map;

};

#endif // GLOBALSTATE_H
