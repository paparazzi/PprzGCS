#include "globalstate.h"

GlobalState* GlobalState::singleton = nullptr;

bool GlobalState::exists(QString key) {
    return map.contains(key);
}

QVariant GlobalState::get(QString key) {
    if(exists(key)) {
        return map[key];
    } else {
        return false;
    }
}

void GlobalState::set(QString key, QVariant value) {
    map[key] = value;
}
