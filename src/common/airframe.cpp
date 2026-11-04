#include "airframe.h"
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <iostream>
#include <algorithm>

QString getAttribute(QDomElement ele, QString name) {
    auto result = ele.attribute(name, "");
    if(result == "") {
        name = name.toUpper();
        result = ele.attribute(name, "");
    }
    return result;
}

Airframe::Airframe(QDomDocument doc, QMap<QString, QDomDocument> includes, QObject* parent):
    QObject(parent), doc(doc), includes(includes)
{
    // Like the airframe expansion of Paparazzi: the children of an included <airframe>
    // replace its <include> node.
    expanded = doc.cloneNode(true).toDocument();
    auto root = expanded.firstChildElement();
    for(auto inc=root.firstChildElement("include"); !inc.isNull();) {
        auto next = inc.nextSiblingElement("include");
        auto href = inc.attribute("href");
        if(includes.contains(href)) {
            auto inc_root = includes[href].firstChildElement();
            for(auto child=inc_root.firstChild(); !child.isNull(); child=child.nextSibling()) {
                root.insertBefore(expanded.importNode(child, true), inc);
            }
        }
        root.removeChild(inc);
        inc = next;
    }

    auto air_root = expanded.firstChildElement( "airframe" );
    name = getAttribute(air_root, "name");

    auto firmware_node = air_root.firstChildElement( "firmware" );
    firmware = getAttribute(firmware_node, "name");
}

// The defines of the sections of a document (the root's <section>s), with the given file.
static QList<Param> sectionParams(const QDomDocument& d, const QString& file) {
    QList<Param> params;
    for(auto section=d.firstChildElement().firstChildElement("section");
        !section.isNull();
        section=section.nextSiblingElement("section")) {
        for(auto define=section.firstChildElement("define");
            !define.isNull();
            define=define.nextSiblingElement("define")) {
            auto full_name = section.attribute("prefix", "") + define.attribute("name");
            params.append({full_name, define.attribute("unit", ""), define.attribute("value"), file});
        }
    }
    return params;
}

// The params of the airframe file, then those of its included files (the ones setParams can save).
QList<Param> Airframe::getParams() {
    QList<Param> params = sectionParams(doc, "");
    for(auto it = includes.begin(); it != includes.end(); ++it) {
        params.append(sectionParams(it.value(), it.key()));
    }
    return params;
}

// Set the values in a document: the defines of its sections with these full names.
static void setSectionValues(QDomDocument& d, const QMap<QString, QString>& values) {
    for(auto section=d.firstChildElement().firstChildElement("section");
        !section.isNull();
        section=section.nextSiblingElement("section")) {
        for(auto define=section.firstChildElement("define");
            !define.isNull();
            define=define.nextSiblingElement("define")) {
            auto full_name = section.attribute("prefix", "") + define.attribute("name");
            if(values.contains(full_name)) {
                define.setAttribute("value", values[full_name]);
            }
        }
    }
}

// A param of the airframe file is set there; one only in an included file, in that file.
void Airframe::setParams(QMap<QString, QString> changed_params) {
    QSet<QString> in_doc;
    for(auto &p: sectionParams(doc, "")) {
        in_doc.insert(p.name);
    }
    QMap<QString, QString> doc_values, inc_values;
    for(auto it = changed_params.begin(); it != changed_params.end(); ++it) {
        (in_doc.contains(it.key()) ? doc_values : inc_values)[it.key()] = it.value();
    }
    setSectionValues(doc, doc_values);
    for(auto it = includes.begin(); it != includes.end(); ++it) {
        setSectionValues(it.value(), inc_values);
    }
}

// In the text of an airframe (or included) file, the value attributes of the defines of its
// sections set to these values; everything else stays as it is. Empty when the text is not read.
static QString replaceValues(const QString& source_text, const QMap<QString, QString>& values) {
    QString text = source_text;
    struct Edit { qsizetype start; qsizetype end; QString value; };
    QList<Edit> edits;
    QXmlStreamReader xml(text);
    int depth = 0;
    QString prefix;
    bool in_section = false;
    while(!xml.atEnd()) {
        auto t = xml.readNext();
        if(t == QXmlStreamReader::StartElement) {
            depth++;
            if(depth == 2 && xml.name() == QStringLiteral("section")) {
                in_section = true;
                prefix = xml.attributes().value("prefix").toString();
            } else if(depth == 3 && in_section && xml.name() == QStringLiteral("define")) {
                auto full_name = prefix + xml.attributes().value("name").toString();
                auto old_value = xml.attributes().value("value").toString();
                if(values.contains(full_name) && values[full_name] != old_value) {
                    qsizetype end = xml.characterOffset();          // just after the start tag
                    qsizetype start = text.lastIndexOf('<', end - 1);
                    edits.append({start, end, values[full_name]});
                }
            }
        } else if(t == QXmlStreamReader::EndElement) {
            if(depth == 2) {
                in_section = false;
            }
            depth--;
        }
    }
    if(xml.hasError()) {
        qDebug() << "Airframe file not read:" << xml.errorString();
        return QString();
    }
    static const QRegularExpression value_re("(\\bvalue\\s*=\\s*)([\"'])(.*?)\\2",
                                             QRegularExpression::DotMatchesEverythingOption);
    for(int i = edits.size() - 1; i >= 0; i--) {     // from the end: the offsets stay valid
        auto &e = edits[i];
        QString tag = text.mid(e.start, e.end - e.start);
        auto m = value_re.match(tag);
        if(!m.hasMatch()) {
            continue;
        }
        QString v = e.value.toHtmlEscaped();
        if(m.captured(2) == "'") {
            v.replace("&quot;", "\"");
        }
        tag.replace(m.capturedStart(3), m.capturedLength(3), v);
        text.replace(e.start, e.end - e.start, tag);
    }
    return text;
}

static QString readText(const QString& path) {
    QFile f(path);
    if(path.isEmpty() || !f.open(QIODevice::ReadOnly)) {
        return QString();
    }
    return QString::fromUtf8(f.readAll());
}

static bool writeText(const QString& path, const QString& text) {
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open" << path << "for writing.";
        return false;
    }
    file.write(text.toUtf8());
    return true;
}

QStringList Airframe::saveIncludes(QMap<QString, QString> paths, bool keep_copy) {
    QStringList written;
    for(auto it = includes.begin(); it != includes.end(); ++it) {
        QString path = paths.value(it.key());
        QString source = readText(path);
        if(source.isEmpty()) {
            continue;
        }
        QMap<QString, QString> values;
        for(auto &p: sectionParams(it.value(), it.key())) {
            values[p.name] = p.value;
        }
        QString text = replaceValues(source, values);
        if(text.isEmpty() || text == source) {
            continue;   // not read, or nothing changed in it
        }
        if(keep_copy) {
            auto suffix = QDateTime::currentDateTime().toString("_yy_MM_dd__hh_mm_ss") + ".xml";
            QFile::copy(path, path.left(path.length() - 4) + suffix);
        }
        if(writeText(path, text)) {
            written.append(path);
        }
    }
    return written;
}

void Airframe::saveSettings(QString filename, QString source) {
    QString text;
    QString source_text = readText(source);
    if(!source_text.isEmpty()) {
        QMap<QString, QString> values;
        for(auto &p: sectionParams(doc, "")) {
            values[p.name] = p.value;
        }
        text = replaceValues(source_text, values);
    }
    if(text.isEmpty()) {
        text = doc.toString();      // not read: written from the document
    }
    writeText(filename, text);
}

QString Airframe::getIconName() {
    auto d = getDefine("AC_ICON", "GCS");
    if(d.has_value()) {
        return d.value().value;
    }

    return firmware;
}

float Airframe::getAltShiftPlus() {
    auto d = getDefine("ALT_SHIFT_PLUS", "GCS");
    if(d.has_value()) {
        return d.value().value.toDouble();
    }

    if(firmware == "fixedwing") {
        return ALT_PLUS_FW;
    } else if (firmware == "rotorcraft") {
        return ALT_PLUS_RC;
    }

    return 0;
}

float Airframe::getAltShiftPlusPlus() {
    auto d = getDefine("ALT_SHIFT_PLUS_PLUS", "GCS");
    if(d.has_value()) {
        return d.value().value.toDouble();
    }

    if(firmware == "fixedwing") {
        return ALT_PLUS_PLUS_FW;
    } else if (firmware == "rotorcraft") {
        return ALT_PLUS_PLUS_RC;
    }

    return 0;
}

float Airframe::getAltShiftMinus() {
    auto d = getDefine("ALT_SHIFT_MINUS", "GCS");
    if(d.has_value()) {
        return d.value().value.toDouble();
    }

    if(firmware == "fixedwing") {
        return ALT_MINUS_FW;
    } else if (firmware == "rotorcraft") {
        return ALT_MINUS_RC;
    }

    return 0;
}

/**
 * @brief Airframe::getDefine
 * @param def_name: full name of the define, including prefix
 * @param section: optional section name
 * @return
 */
std::optional<Param> Airframe::getDefine(QString def_name, QString section_name) {
    for(auto section=expanded.firstChildElement().firstChildElement("section");
        !section.isNull();
        section=section.nextSiblingElement("section")) {
        if(section_name != "" && section_name != section.attribute("name", "")) {
            continue;
        }
        for(auto define=section.firstChildElement("define");
            !define.isNull();
            define=define.nextSiblingElement("define")) {
            auto full_name = section.attribute("prefix", "") + define.attribute("name");
            if(full_name == def_name) {
                auto value = define.attribute("value");
                auto unit = define.attribute("unit", "");
                struct Param ret =  {full_name, unit, value, QString()};
                return ret;
            }
        }
    }
    return std::nullopt;
}

QList<ChecklistItem*> Airframe::getChecklistItems() {
    QList<ChecklistItem*> items;

    for(auto section=expanded.firstChildElement().firstChildElement("checklist");
        !section.isNull();
        section=section.nextSiblingElement("checklist")) {
        for(auto item=section.firstChildElement("item");
            !item.isNull();
            item=item.nextSiblingElement("item")) {
            auto name = item.attribute("name");
            auto description = item.firstChild().nodeValue();
            auto type = item.attribute("type", "checkbox");
            auto checklist_item = new ChecklistItem();
            checklist_item->name = name;
            checklist_item->description = description;
            checklist_item->type = type;
            checklist_item->optional = item.attribute("optional", "false") == "true";
            checklist_item->help = item.attribute("help");
            checklist_item->remember = item.attribute("remember", "false") == "true";
            if(item.hasAttribute("values")) {
                checklist_item->values = item.attribute("values").split("|");
            }

            items.append(checklist_item);
        }
    }
    return items;
}
