#include "plotter.h"

#include <QtWidgets>
#include "AircraftManager.h"
#include <QDebug>

Plotter::Plotter(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id)
{
    auto lay = new QVBoxLayout(this);

    auto top_lay = new QHBoxLayout();
    lay->addItem(top_lay);

    title = new QLabel("title", this);
    title->setStyleSheet("font-weight: bold;");
    top_lay->addWidget(title, 0, Qt::AlignHCenter);
    top_lay->setStretch(0, 1);

    autoscale_checkbox = new QCheckBox("autoscale", this);
    top_lay->addWidget(autoscale_checkbox);
    connect(autoscale_checkbox, &QCheckBox::stateChanged, this, [=](int state) {
        if(graphs.contains(current_name)) {
            auto p = graphs[current_name]->getParams();
            p.autoscale = state;
            graphs[current_name]->setParams(p);
        }
    });

    history_spinbox = new QSpinBox(this);
    top_lay->addWidget(history_spinbox);
    history_spinbox->setRange(1, 999);
    history_spinbox->setToolTip("history (s)");
    connect(history_spinbox, qOverload<int>(&QSpinBox::valueChanged), this, [=](int value) {
        if(graphs.contains(current_name)) {
            graphs[current_name]->setHistory(value * 1000);
        }
    });

    var_button = new QToolButton(this);
    var_button->setIcon(style()->standardIcon(QStyle::SP_TitleBarUnshadeButton));
    top_lay->addWidget(var_button);

    auto close_button = new QToolButton(this);
    close_button->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    close_button->setToolTip("Close current graph");
    top_lay->addWidget(close_button);

    graph_stack = new QStackedWidget(this);
    lay->addWidget(graph_stack);
    lay->setStretch(1, 1);

    connect(var_button, &QToolButton::clicked, this, &Plotter::onOpenContextMenu);
    connect(close_button, &QToolButton::clicked, this, [=]() {removeGraph(current_name);});


    setAcceptDrops(true);
}

void Plotter::configure(QDomElement c) {

    for(auto node=c.firstChildElement(); !node.isNull(); node=node.nextSiblingElement()) {
        if(node.tagName() != "plot" || !node.hasAttribute("name")) {
            continue;
        }

        auto name = node.attribute("name", "");
        auto def = name.split(":");

        if(def.size() != 3) {
            continue;
        }

        GraphWidget::Params p = {100, 0, true};

        if(node.hasAttribute("min")) {
            p.min = node.attribute("min").toDouble();
            p.autoscale = false;
        }
        if(node.hasAttribute("max")) {
            p.max = node.attribute("max").toDouble();
            p.autoscale = false;
        }
        addGraph(name, p);
    }
}

void Plotter::addGraph(QString name, GraphWidget::Params p) {
    auto graph = new GraphWidget(this);
    graph_stack->addWidget(graph);
    graphs[name] = graph;

    auto def = name.split(":");

    bids[name] = PprzDispatcher::get()->bind(def[1], this,
        [=](QString sender, pprzlink::Message msg){
            handleMsg(name, sender, msg);
        });

    graph->setParams(p);

    if(current_name.isNull()) {
        current_name = name;
    }

    connect(graph, &GraphWidget::autoscaleChanged, this, [=](bool a){
        if(graph == graphs[current_name]) {
            autoscale_checkbox->setChecked(a);
        }
    });

    changeGraph(current_name);
}

void Plotter::removeGraph(QString name) {
    if(graphs.contains(name)) {
        PprzDispatcher::get()->unBind(bids[name]);
        auto graph = graphs[name];
        graphs.remove(name);
        graph->deleteLater();
        if(!graphs.isEmpty()) {
            changeGraph(graphs.firstKey());
        } else {
            changeGraph(QString::Null());
        }
    }
}

void Plotter::changeGraph(QString name) {
    current_name = name;
    if(graphs.contains(current_name)) {
        graph_stack->setCurrentWidget(graphs[current_name]);
        title->setText(current_name.split(":")[2]);
        autoscale_checkbox->setChecked(graphs[current_name]->getParams().autoscale);
        history_spinbox->setValue(graphs[current_name]->getHistory()/1000);
    } else {
        title->setText("no graph...");
    }
}

void Plotter::onOpenContextMenu() {
    auto menu = new QMenu(this);

    for(auto gr=graphs.begin(); gr!=graphs.end(); ++gr) {
        auto action = menu->addAction(gr.key());
        connect(action, &QAction::triggered, this, [=]() {
            changeGraph(gr.key());
        });
    }

    auto groundMenu = menu->addMenu("ground");

    auto msg_dict = PprzDispatcher::get()->getDict();
    for(auto &def: msg_dict->getMsgsForClass("ground")) {
        auto msg_menu = groundMenu->addMenu(def.getName());
        for(size_t i=0; i < def.getNbFields(); ++i) {
            auto f = def.getField(static_cast<int>(i));
            auto f_action = msg_menu->addAction(f.getName());
            connect(f_action, &QAction::triggered, this, [=]() {
                auto name = "ground:" +  def.getName() + ":" + f.getName();
                addGraph(name, {100, 0, true});
                changeGraph(name);
            });
        }
    }

    auto pos = var_button->pos() + var_button->rect().center();
    pos = mapToGlobal(pos);

    menu->popup(pos);
}


void Plotter::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat("text/plain")) {
        event->acceptProposedAction();
    }
}

void Plotter::dragMoveEvent(QDragMoveEvent *event) {
    event->accept();
}

/**
 * @brief Plotter::dropEvent handle drop for mime type text/plain
 * @param event
 * Paparazzi messages : "<id>:<class>:<msg name>:<field>:<???>" ex: "2:telemetry:WP_MOVED:utm_north:1."
 */
void Plotter::dropEvent(QDropEvent *event) {
    QString text = event->mimeData()->text();
    QStringList args = text.split(QString(":"));

    QRegularExpression pprz_msg_re("^(\\w+):(\\w+):(\\w+):(\\w+):.*$");
    QRegularExpressionMatch pprz_msg_match = pprz_msg_re.match(text);

    if(pprz_msg_match.hasMatch()) {
        // Paparazzi message, ex: "2:telemetry:WP_MOVED:utm_north:1."
        QString id = pprz_msg_match.captured(1);
        QString msg_class = pprz_msg_match.captured(2);
        QString msg_name = pprz_msg_match.captured(3);
        QString field = pprz_msg_match.captured(4);

        if(id == ac_id) {
            //qDebug() << id << msg_class << msg_name << field;;
            auto name = QString("%1:%2:%3").arg(msg_class, msg_name, field);
            addGraph(name, {100, 0, true});
            changeGraph(name);
        }
    }
}


void Plotter::handleMsg(QString name, QString sender, pprzlink::Message msg) {
    auto def = name.split(":");
    auto msg_class = def[0];
    auto field = def[2];

    if(msg_class == "ground") {
        QString id;
        try {
            msg.getField("ac_id", id);
            if(ac_id != id) {
                return;
            }
        }  catch (pprzlink::no_such_field& e) {
            return;
        }
    } else if(msg_class == "telemetry") {
        if(sender != ac_id) {
            return;
        }
    }

    auto t = msg.getDefinition().getField(field).getType();
    auto bt = t.getBaseType();

    if(!t.isArray()) {
        if(bt == pprzlink::BaseType::FLOAT) {
            feedGraph<float>(graphs[name], field, msg);
        } else if(bt == pprzlink::BaseType::DOUBLE) {
            feedGraph<double>(graphs[name], field, msg);
        } else if(bt == pprzlink::BaseType::UINT8) {
            feedGraph<uint8_t>(graphs[name], field, msg);
        } else if(bt == pprzlink::BaseType::UINT16) {
            feedGraph<uint16_t>(graphs[name], field, msg);
        } else if(bt == pprzlink::BaseType::UINT32) {
            feedGraph<uint32_t>(graphs[name], field, msg);
        } else if(bt == pprzlink::BaseType::INT8) {
            feedGraph<int8_t>(graphs[name], field, msg);
        } else if(bt == pprzlink::BaseType::INT16) {
            feedGraph<int16_t>(graphs[name], field, msg);
        } else if(bt == pprzlink::BaseType::INT32) {
            feedGraph<int32_t>(graphs[name], field, msg);
        } else {
            qDebug() << "type not supported yet";
        }
    } else {
        qDebug() << "arrays not supported yet";
    }
}

template<typename T>
void Plotter::feedGraph(GraphWidget* graph, QString field, pprzlink::Message msg) {
    T val;
    msg.getField(field, val);
    graph->pushData(val);
}

