#ifndef AIRFRAME_H
#define AIRFRAME_H

#include <QString>
#include <QtXml>

struct Param {
    QString name;
    QString unit;
    QString value;
};

class Airframe: public QObject
{
    Q_OBJECT
public:

    struct Define {
        QString name;
        QString value;
        //...
    };

    struct Section {
      QString name;
      QString prefix;
      QList<struct Define> defines;
      //...
    };

    Airframe(QDomDocument doc, QObject* parent=nullptr);

    QString getFirmware() {return firmware;}
    QString getIconName();
    float getAltShiftPlus();
    float getAltShiftPlusPlus();
    float getAltShiftMinus();

    void saveSettings(QString filename);
    void setParams(QMap<QString, QString> changed_params);
    QList<Param> getParams();

private:
    QString name;
    QString firmware;

    QDomDocument doc;

    QList<struct Section> sections;

    static constexpr float ALT_PLUS_FW = 10;
    static constexpr float ALT_MINUS_FW = -10;
    static constexpr float ALT_PLUS_PLUS_FW = 30;

    static constexpr float ALT_PLUS_RC = 1;
    static constexpr float ALT_MINUS_RC = -1;
    static constexpr float ALT_PLUS_PLUS_RC = 3;
};

#endif // AIRFRAME_H
