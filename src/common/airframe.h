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
    float getAltShiftPlus();
    float getAltShiftPlusPlus();
    float getAltShiftMinus();

private:
    string name;
    string firmware;

    vector<struct Section> sections;

    static constexpr float ALT_PLUS_FW = 10;
    static constexpr float ALT_MINUS_FW = -10;
    static constexpr float ALT_PLUS_PLUS_FW = 30;

    static constexpr float ALT_PLUS_RC = 1;
    static constexpr float ALT_MINUS_RC = -1;
    static constexpr float ALT_PLUS_PLUS_RC = 3;
};

#endif // AIRFRAME_H
