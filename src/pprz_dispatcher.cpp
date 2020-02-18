#include "pprz_dispatcher.h"
#include <iostream>

using namespace std;

PprzDispatcher* PprzDispatcher::singleton = nullptr;


void ivyMessageCallback(std::string ac_id, pprzlink::Message msg)
{
    (void)ac_id;
    (void)msg;
  //std::cout << ac_id << " sent " << msg.toString() << std::endl;
}


PprzDispatcher::PprzDispatcher(QObject *parent) : QObject (parent)
{
    dict = std::make_unique<pprzlink::MessageDictionary>("/home/fabien/paparazzi/var/messages.xml");
    link = std::make_unique<pprzlink::IvyLink>(*dict, "TestPprzGCS", "127.255.255.255:2010", true);

//pprzlink::MessageDictionary
    link->BindMessage(dict->getDefinition("GPS"), ivyMessageCallback);
}


