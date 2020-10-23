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
    string getName() { if(shortname != "") { return shortname;} else {return var;}}
    vector<string>& getValues() {return values;}
    tuple<float, float, float> getBounds() {return make_tuple(min, max, step);}
    void setValue(float v) {
        last_values[1] = last_values[0];
        last_values[0] = v;
    }

    float getValue() {return last_values[0];}
    float getPreviousValue() {return last_values[1];}
    float getMin() {return  min;}
    float getMax() {return  max;}


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

    float last_values[2];
};

#endif // SETTING_H
