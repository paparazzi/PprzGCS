#include "pprz_dispatcher.h"
#include <iostream>
using namespace std;

PprzDispatcher* PprzDispatcher::singleton = nullptr;

PprzDispatcher::PprzDispatcher(QObject *parent) : QObject (parent)
{

}
