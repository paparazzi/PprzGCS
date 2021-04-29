#ifndef AIRFRAME_H
#define AIRFRAME_H

#include <QString>
#include <QtXml>
#include <vector>

using namespace std;

class Airframe
{
public:

    struct Define {
        QString name;
        QString value;
        //...
    };

    struct Section {
      QString name;
      QString prefix;
      vector<struct Define> defines;
      //...
    };

    Airframe();
    Airframe(QDomDocument doc);

    QString getFirmware() {return firmware;}
    QString getIconName();
    float getAltShiftPlus();
    float getAltShiftPlusPlus();
    float getAltShiftMinus();

private:
    QString name;
    QString firmware;

    vector<struct Section> sections;

    static constexpr float ALT_PLUS_FW = 10;
    static constexpr float ALT_MINUS_FW = -10;
    static constexpr float ALT_PLUS_PLUS_FW = 30;

    static constexpr float ALT_PLUS_RC = 1;
    static constexpr float ALT_MINUS_RC = -1;
    static constexpr float ALT_PLUS_PLUS_RC = 3;
};

#endif // AIRFRAME_H
