#include "listcontainer.h"
#include "dispatcher_ui.h"
#include "AircraftManager.h"
#include "strip.h"

ListContainer::ListContainer(std::function<QWidget*(QString, QWidget*)> constructor, QWidget *parent) : QWidget(parent),
    constructor(constructor)
{
    scroll = new QScrollArea(this);
    auto scroll_content = new QWidget(scroll);
    scroll->setWidget(scroll_content);
    scroll->setWidgetResizable(true);
    scroll->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto vbox = new QVBoxLayout(scroll_content);
    grid_layout = new QGridLayout();
    vbox->addLayout(grid_layout);
    vbox->addStretch(1);

    auto main_layout  = new QVBoxLayout(this);
    main_layout->addWidget(scroll);

    // FIXME: small hack to set minimum size of the scrollarea
    QTimer* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [=]() {
        scroll->setMinimumWidth(grid_layout->sizeHint().width());
    });
    t->start(500);

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &ListContainer::handleNewAC);
    connect(DispatcherUi::get(), &DispatcherUi::ac_deleted, this, &ListContainer::removeAC);
}

void ListContainer::handleNewAC(QString ac_id) { 
    QWidget* pageWidget;
    try {
        pageWidget = constructor(ac_id, this);
        if(!conf.isNull()) {
            Configurable* c = dynamic_cast<Configurable*>(pageWidget);
            if(c != nullptr) {
                c->configure(conf);
            } else {
                throw std::runtime_error("Widget is not Configurable!!!");
            }
        }
    }  catch (std::runtime_error &e) {
        auto msg = QString(e.what());
        pageWidget = new QWidget(this);
        auto lay = new QVBoxLayout(pageWidget);
        auto label = new QLabel(msg, pageWidget);
        label->setWordWrap(true);
        lay->addWidget(label);
        lay->addStretch();
    }
    widgets[ac_id] = pageWidget;
    auto row = grid_layout->rowCount();
    grid_layout->addWidget(pageWidget, row, 1);


    auto ac = AircraftManager::get()->getAircraft(ac_id);
    auto color_rect = new QWidget(this);
    color_rect->setMinimumSize(QSize(20, 20));
    color_rect->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    color_rect->setStyleSheet("background: " + ac->getColor().name());
    grid_layout->addWidget(color_rect, row, 0);
    color_rect->installEventFilter(this);
    rects[color_rect] = ac_id;
}

void ListContainer::removeAC(QString ac_id) {
    grid_layout->removeWidget(widgets[ac_id]);
    widgets[ac_id]->deleteLater();
    widgets.remove(ac_id);

    auto rect = std::find(rects.begin(), rects.end(), ac_id);
    if(rect != rects.end()) {
        grid_layout->removeWidget(rect.key());
        rect.key()->deleteLater();
        rects.remove(rect.key());
    }
}

bool ListContainer::eventFilter(QObject *object, QEvent *event)
{
    auto widget = dynamic_cast<QWidget*>(object);
    if (event->type() == QEvent::MouseButtonPress && rects.contains(widget)) {
        emit DispatcherUi::get()->ac_selected(rects[widget]);
        return true;
    }
    return false;
}
