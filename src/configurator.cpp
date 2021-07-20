#include "configurator.h"
#include <QtXml>
#include <QFile>
#include <QPushButton>
#include <QSplitter>
#include <QMainWindow>
#include <iostream>
#include "pprzmain.h"
#include "pprzmap.h"
#include "pfd.h"
#include <QLabel>
#include <iostream>
#include "configurable.h"
#include "widget_utils.h"
#include "speaker.h"

static const char* DEFAULT_WIDTH = "1024";
static const char* DEFAULT_HEIGHT = "600";

QWidget* rec_layout_build(QDomElement &ele, QSplitter* parent, int* size) {
    *size = ele.attribute("size", "10").toInt();
    if(ele.tagName() == "rows" or ele.tagName()=="columns") {
        QSplitter* splitter = new QSplitter(parent);
        splitter->setChildrenCollapsible(false);
        if(ele.tagName()=="rows") {
            splitter->setOrientation(Qt::Vertical);
        } else{
            splitter->setOrientation(Qt::Horizontal);
        }

        QList<int> sizes;
        for(auto child=ele.firstChildElement(); !child.isNull(); child=child.nextSiblingElement()) {
            int s = 0;
            rec_layout_build(child, splitter, &s);
            sizes.append(s);
        }
        splitter->setSizes(sizes);

        if(parent != nullptr) {
            parent->addWidget(splitter);
        } else {
            return splitter;
        }

    } else if(ele.tagName()=="widget") {
        QString name = ele.attribute("name", "");
        auto container = ele.attribute("container", "stack");
        if(name == "") {
            throw invalid_widget_name("Invalid widget name");
        }

        QWidget* widget = makeWidget(name, container, parent);

        for(auto layout_ele=ele.firstChildElement(); !layout_ele.isNull(); layout_ele=layout_ele.nextSiblingElement()) {
            if(layout_ele.tagName() == "configure") {
                Configurable* c = dynamic_cast<Configurable*>(widget);
                if(c != nullptr) {
                    c->configure(layout_ele);
                } else {
                    throw std::runtime_error("Class does not inherit from Configurable!!!");
                }
            } else {
                throw std::runtime_error("Unhandled tag " + layout_ele.tagName().toStdString());
            }
        }

        if(parent != nullptr) {
            parent->addWidget(widget);
        } else {
            return widget;
        }
    } else {
        throw invalid_tag("Invalid tag: the tag should be one of [rows, columns, widget]");
    }
    return nullptr;
}

void configure_speech(QDomElement ele) {
    if(ele.isNull()) {
        return;
    }

    if(ele.hasAttribute("locale")) {
        auto locale = ele.attribute("locale");
        pprzApp()->toolbox()->speaker()->setLocale(locale);
    }

    for(auto m=ele.firstChildElement(); !m.isNull(); m=m.nextSiblingElement()) {
        auto name = m.attribute("name");
        auto txt = m.attribute("text");
        auto period = m.attribute("timeout").toInt();
        auto priority = m.attribute("priority", "0").toInt();
        auto expire = m.attribute("expire", "30").toInt();
        auto onChangeStr = m.attribute("onChange", "false");
        auto postprocessing = m.attribute("postprocessing", "");
        bool onChange = false;
        if(onChangeStr == "1" || onChangeStr == "true" || onChangeStr == "si senor") {
            onChange = true;
        }
        pprzApp()->toolbox()->speaker()->registerMessage(name, txt, period, priority, expire, onChange, postprocessing);
    }
}

PprzMain* configure(QString filename) {
    QDomDocument xmlLayout;
    QFile f(filename);
    if(!f.open(QIODevice::ReadOnly)) {
        throw file_error("Error while loading layout file");
    }
    xmlLayout.setContent(&f);
    f.close();

    QDomElement root = xmlLayout.documentElement();

    // configure layout
    auto layout_ele = root.firstChildElement("layout");
    if(layout_ele.isNull()) {
        throw invalid_tag("Cannot find a \"layout\" element.");
    }

    int width = layout_ele.attribute("width", DEFAULT_WIDTH).toInt();
    int height = layout_ele.attribute("height", DEFAULT_HEIGHT).toInt();

    if (layout_ele.childNodes().length() != 1) {
        throw invalid_tag("Root tag \"layout\" expected to have exactly one child, rows, colums or widget.");
    }

    QDomElement first_element = layout_ele.firstChildElement();
    int s;
    QWidget* widget = rec_layout_build(first_element, nullptr, &s);

    auto window = pprzApp()->mainWindow();
    window->setupUi(width, height, widget);


    //configure speech
    configure_speech(root.firstChildElement("speech"));


    return window;
}
