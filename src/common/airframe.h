#ifndef AIRFRAME_H
#define AIRFRAME_H

#include <QString>
#include <QtXml>
#include <optional>

struct Param {
    QString name;
    QString unit;
    QString value;
};

struct ChecklistItem {
    QString name;
    QString description;
    QString type;           // checkbox, text or choice
    QString value;
    bool optional = false;  // not needed to finish the checklist
    QString help;           // longer instructions, shown as tooltip
    bool remember = false;  // pre-fill with the value of the previous flight
    QStringList values;     // choices of a "choice" item
    bool logged = false;    // the current value has been sent
};

class Airframe: public QObject
{
    Q_OBJECT
public:

    /**
     * @param doc: the airframe file
     * @param includes: the files of its <include> nodes, by href (see ConfigData)
     */
    Airframe(QDomDocument doc, QMap<QString, QDomDocument> includes = {}, QObject* parent=nullptr);

    QString getFirmware() {return firmware;}
    QString getIconName();
    float getAltShiftPlus();
    float getAltShiftPlusPlus();
    float getAltShiftMinus();
    std::optional<struct Param> getDefine(QString name, QString section="");

    /// Write the airframe file with the values set by setParams. From the text of source (the file
    /// it was loaded from), only the changed value attributes replaced: the layout, comments and
    /// the rest of the file stay as they are. Without source: the whole document is written.
    void saveSettings(QString filename, QString source = "");
    void setParams(QMap<QString, QString> changed_params);
    QList<Param> getParams();
    QList<ChecklistItem*> getChecklistItems();

private:
    QString name;
    QString firmware;

    QDomDocument doc;       // the airframe file itself: the one saved by saveSettings
    QDomDocument expanded;  // with the <include> nodes replaced by the included content

    static constexpr float ALT_PLUS_FW = 10;
    static constexpr float ALT_MINUS_FW = -10;
    static constexpr float ALT_PLUS_PLUS_FW = 30;

    static constexpr float ALT_PLUS_RC = 1;
    static constexpr float ALT_MINUS_RC = -1;
    static constexpr float ALT_PLUS_PLUS_RC = 3;
};

#endif // AIRFRAME_H
