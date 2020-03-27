#ifndef SETTING_H
#define SETTING_H

#include <string>
#include <vector>
#include <memory>
#include "tinyxml2.h"

using namespace tinyxml2;
using namespace std;

class Setting
{
public:

    struct KeyPress {
        string key;
        float value;
        friend ostream& operator<<(ostream& os, const KeyPress& wp);
    };

    struct StripButton {
        string name;
        string group;
        float value;
        string icon;
        uint8_t setting_no;
        friend ostream& operator<<(ostream& os, const StripButton& wp);
    };

    Setting(XMLElement* setel, uint8_t& setting_no);
    vector<shared_ptr<KeyPress>> getKeyPresses() {return key_presses;}
    vector<shared_ptr<StripButton>> getStripButtons() {return strip_buttons;}
    uint8_t getNo() {return setting_no;}


    friend ostream& operator<<(ostream& os, const Setting& wp);

private:
    uint8_t setting_no;

    string var;
    float min;
    float max;
    float step;

    string shortname;
    vector<string> values;
    string module;
    string handler;
    string type;
    string persistent;
    string param;
    string unit;
    string alt_unit;
    string alt_unit_coef;

    vector<shared_ptr<KeyPress>> key_presses;
    vector<shared_ptr<StripButton>> strip_buttons;
};

#endif // SETTING_H
