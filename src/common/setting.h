#ifndef SETTING_H
#define SETTING_H

#include <QString>
#include <vector>
#include <memory>
#include <optional>
#include <QtXml>
#include <QObject>

using namespace std;

class Setting: public QObject
{
    Q_OBJECT
public:

    struct KeyPress {
        QString key;
        float value;
        friend ostream& operator<<(ostream& os, const KeyPress& wp);
    };

    struct StripButton {
        QString name;
        QString group;
        float value;
        QString icon;
        uint8_t setting_no;
        friend ostream& operator<<(ostream& os, const StripButton& wp);
    };

    Setting(QDomElement setel, uint8_t& setting_no, QObject* parent);
    vector<shared_ptr<KeyPress>> getKeyPresses() {return key_presses;}
    vector<shared_ptr<StripButton>> getStripButtons() {return strip_buttons;}
    uint8_t getNo() {return setting_no;}
    QString getName() { if(shortname != "") { return shortname;} else {return var;}}
    vector<QString>& getValues() {return values;}
    tuple<float, float, float> getBounds() {return make_tuple(min, max, step);}
    void setValue(float v) {
        last_set_values[1] = last_set_values[0];
        last_set_values[0] = v;
    }

    float getValue() {return last_set_values[0];}
    float getPreviousValue() {return last_set_values[1];}
    float getMin() {return  min;}
    float getMax() {return  max;}
    optional<float> getInitialValue(){return initial_value;}
    void setInitialValue(float val) {initial_value = val;}

    float getAltUnitCoef();

    friend ostream& operator<<(ostream& os, const Setting& wp);

private:
    uint8_t setting_no;

    QString var;
    float min;
    float max;
    float step;

    QString shortname;
    vector<QString> values;
    QString module;
    QString handler;
    QString type;
    QString persistent;
    QString param;
    QString unit;
    QString alt_unit;
    optional<float> alt_unit_coef;

    vector<shared_ptr<KeyPress>> key_presses;
    vector<shared_ptr<StripButton>> strip_buttons;

    float last_set_values[2];   // last values set by the user from this app.
    optional<float> initial_value;
};

#endif // SETTING_H
