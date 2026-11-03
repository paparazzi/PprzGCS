#include "airframe.h"
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <QFile>
#include <QDebug>
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
    QObject(parent), doc(doc)
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

// Only the params of the airframe file itself, as these are the ones setParams can save.
QList<Param> Airframe::getParams() {
    QList<Param> params;

    for(auto section=doc.firstChildElement().firstChildElement("section");
        !section.isNull();
        section=section.nextSiblingElement("section")) {
        for(auto define=section.firstChildElement("define");
            !define.isNull();
            define=define.nextSiblingElement("define")) {
            auto full_name = section.attribute("prefix", "") + define.attribute("name");
            auto value = define.attribute("value");
            auto unit = define.attribute("unit", "");
            params.append({full_name, unit, value});
        }
    }
    return params;
}

void Airframe::setParams(QMap<QString, QString> changed_params) {
    for(auto section=doc.firstChildElement().firstChildElement("section");
        !section.isNull();
        section=section.nextSiblingElement("section")) {
        for(auto define=section.firstChildElement("define");
            !define.isNull();
            define=define.nextSiblingElement("define")) {
            auto full_name = section.attribute("prefix", "") + define.attribute("name");

            for(auto it = changed_params.begin(); it!=changed_params.end(); ++it) {
                if(full_name == it.key()) {
                    define.setAttribute("value", it.value());
                }
            }
        }
    }
}

void Airframe::saveSettings(QString filename, QString source) {
    QString text;
    QFile src(source);
    if(!source.isEmpty() && src.open(QIODevice::ReadOnly)) {
        text = QString::fromUtf8(src.readAll());
        src.close();
        // the values to write: the defines of the document (setParams changed them)
        QMap<QString, QString> values;
        for(auto &p: getParams()) {
            values[p.name] = p.value;
        }
        // the defines of the sections of the root, found in the text with their offsets; the
        // value attribute replaced in the text of their start tag (from the end: offsets stay valid)
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
            qDebug() << "Airframe file not read, written from the document:" << xml.errorString();
            text.clear();
        } else {
            static const QRegularExpression value_re("(\\bvalue\\s*=\\s*)([\"'])(.*?)\\2",
                                                     QRegularExpression::DotMatchesEverythingOption);
            for(int i = edits.size() - 1; i >= 0; i--) {
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
        }
    }
    if(text.isEmpty()) {
        text = doc.toString();
    }

    QFile file(filename);
    if( !file.open( QIODevice::WriteOnly ) )
    {
        qDebug( "Failed to open file for writing." );
        return;
    }
    file.write(text.toUtf8());
    file.close();
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
                struct Param ret =  {full_name, unit, value};
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
