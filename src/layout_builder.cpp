#include "layout_builder.h"
#include <QtXml>
#include <QFile>
#include <QPushButton>
#include <QSplitter>
#include <QMainWindow>
#include <iostream>
#include "pprzmain.h"
#include "strip.h"
#include "map2d.h"
#include "ac_selector.h"
#include <QLabel>

#include <iostream>

static const char* DEFAULT_WIDTH = "1024";
static const char* DEFAULT_HEIGHT = "600";

QWidget* rec_build(QDomNode &node, QSplitter* parent, int* size) {
    if(!node.isElement()) {
        throw invalid_node("node is not an Element.");
    } else {
        QDomElement ele = node.toElement();
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
            for(int i=0; i<ele.childNodes().length(); i++) {
                QDomNode child = ele.childNodes().item(i);
                int s = 0;
                rec_build(child, splitter, &s);
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
            if(name == "") {
                throw invalid_widget_name("Invalid widget name");
            }

            QWidget* widget;
            if (name == "strips") {
                widget = new Strip(parent);
            } else if (name == "alarms") {
                widget = new ACSelector(parent);
            } else if(name == "map2d") {
                widget = new Map2D(parent);
            } else if (name == "aircraft" or name=="altgraph") {
                widget = new QWidget(); // dummy widget
            } else {
                std::string s = "Widget " + name.toStdString() + " unknown";
                throw unknown_widget(s);
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
}

PprzMain* build_layout(QString filename) {
    QDomDocument xmlLayout;
    QFile f(filename);
    if(!f.open(QIODevice::ReadOnly)) {
        throw file_error("Error while loading layout file");
    }
    xmlLayout.setContent(&f);
    f.close();

    QDomElement root = xmlLayout.documentElement();
    QString rootTag = root.tagName();
    if (rootTag != "layout") {
        throw invalid_tag("Root tag expected to be \"layout\". Is this a layout file ?");
    }

    int width = root.attribute("width", DEFAULT_WIDTH).toInt();
    int height = root.attribute("height", DEFAULT_HEIGHT).toInt();

    if (root.childNodes().length() != 1) {
        throw invalid_tag("Root tag \"layout\" expected to have exactly one child, rows, colums or widget.");
    }

    QDomNode root_node = root.childNodes().item(0);
    int s;
    QWidget* widget = rec_build(root_node, nullptr, &s);

    PprzMain *w = new PprzMain(width, height, widget);

    return w;
}
