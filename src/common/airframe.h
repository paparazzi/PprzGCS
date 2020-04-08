#ifndef AIRFRAME_H
#define AIRFRAME_H

#include <string>
#include "tinyxml2.h"
#include <vector>

using namespace std;
using namespace tinyxml2;

class Airframe
{
public:

    struct Define {
        string name;
        string value;
        //...
    };

    struct Section {
      string name;
      string prefix;
      vector<struct Define> defines;
      //...
    };

    Airframe();
    Airframe(std::string uri);

    string getFirmware() {return firmware;}
    string getIconName();

private:
    string name;
    string firmware;

    vector<struct Section> sections;
};

#endif // AIRFRAME_H
