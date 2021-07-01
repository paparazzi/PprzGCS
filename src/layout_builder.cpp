#include "layout_builder.h"
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

static const char* DEFAULT_WIDTH = "1024";
static const char* DEFAULT_HEIGHT = "600";

QWidget* rec_build(QDomNode &node, QSplitter* parent, int* size) {
    if(!node.isElement()) {
        return nullptr;
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
            auto container = ele.attribute("container", "stack");
            if(name == "") {
                throw invalid_widget_name("Invalid widget name");
            }

            QWidget* widget = makeWidget(name, container, parent);

            for(int i=0; i<ele.childNodes().length(); i++) {
                QDomNode node = ele.childNodes().item(i);
                if(!node.isElement()) {
                    continue;
                }
                QDomElement layout_ele = node.toElement();
                if(layout_ele.tagName() == "configure") {
                    Configurable* c = dynamic_cast<Configurable*>(widget);
                    if(c == nullptr) {
                        throw std::runtime_error("Class does not inherit from Configurable!!!");
                    }

                    c->configure(layout_ele);
                } else if (layout_ele.tagName() == "horizontalLayout" || layout_ele.tagName() == "verticalLayout") {
                    QLayout* layout = layout_rec_build(layout_ele, widget);
                    widget->setLayout(layout);
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
}

QLayout* layout_rec_build(QDomElement &ele, QWidget* parent) {
    (void)parent;

    QBoxLayout* layout;
    if(ele.tagName() == "horizontalLayout") {
        layout = new QHBoxLayout();
    } else if (ele.tagName() == "verticalLayout") {
        layout = new QVBoxLayout();
    }
    else {
        throw std::runtime_error("layout must be either verticalLayout or horizontalLayout");
    }

    for(int i=0; i<ele.childNodes().length(); i++) {
        QDomNode child = ele.childNodes().item(i);
        if(!child.isElement()) {
            throw invalid_node("layout node is not an Element.");
        }
        auto child_ele = child.toElement();
        if(child_ele.tagName() == "horizontalLayout" || child_ele.tagName() == "verticalLayout") {
            auto child_layout = layout_rec_build(child_ele, parent);
            layout->addItem(child_layout);
        }
        else if(child_ele.tagName() == "widget") {
            auto name = child_ele.attribute("name");
            auto container = child_ele.attribute("container", "stack");
            QWidget* widget = makeWidget(name, container, parent);
            layout->addWidget(widget);
        }
        else if (child_ele.tagName() == "spacer") {
            layout->addStretch();
        }

    }

    return layout;
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

    auto window = pprzApp()->mainWindow();
    window->setupUi(width, height, widget);

    return window;
}
