#ifndef AIRFRAME_H
#define AIRFRAME_H

#include <QString>
#include <QtXml>
#include <optional>

struct Param {
    QString name;
    QString unit;
    QString value;
    QString file;   // the href of the included file it is in; empty: the airframe file itself
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
    /// A battery alarm level of the pack, in V: LOW_BAT_LEVEL, CRITIC_BAT_LEVEL or CATASTROPHIC_BAT_LEVEL.
    /// Not set: the default per cell (defaultCellLevel) times BAT_NB_CELLS, else 3 cells (as on board
    /// and in the server; not the cells guessed from the voltage, so the GCS warns at the same voltages).
    double batteryLevel(QString name);
    /// The default of a battery alarm level per cell (V): LOW 3.5, CRITIC 3.25, CATASTROPHIC 3.0
    static double defaultCellLevel(QString name);

    /// Write the airframe file with the values set by setParams. From the text of source (the file
    /// it was loaded from), only the changed value attributes replaced: the layout, comments and
    /// the rest of the file stay as they are. Without source: the whole document is written.
    void saveSettings(QString filename, QString source = "");
    /// Write the included files whose values setParams changed, in place (path: by href), the same
    /// way (only the values change). Returns the paths written.
    QStringList saveIncludes(QMap<QString, QString> paths, bool keep_copy);
    void setParams(QMap<QString, QString> changed_params);
    QList<Param> getParams();
    QList<ChecklistItem*> getChecklistItems();

private:
    QString name;
    QString firmware;

    QDomDocument doc;       // the airframe file itself: the one saved by saveSettings
    QMap<QString, QDomDocument> includes;     // its included files, by href (saveIncludes)
    QDomDocument expanded;  // with the <include> nodes replaced by the included content

    static constexpr float ALT_PLUS_FW = 10;
    static constexpr float ALT_MINUS_FW = -10;
    static constexpr float ALT_PLUS_PLUS_FW = 30;

    static constexpr float ALT_PLUS_RC = 1;
    static constexpr float ALT_MINUS_RC = -1;
    static constexpr float ALT_PLUS_PLUS_RC = 3;
};

#endif // AIRFRAME_H
