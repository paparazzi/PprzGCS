#include "listcontainer.h"
#include "dispatcher_ui.h"
#include "AircraftManager.h"
#include "strip.h"

ListContainer::ListContainer(std::function<QWidget*(QString, QWidget*)> constructor, QWidget *parent) : QWidget(parent),
    constructor(constructor)
{
    scroll = new QScrollArea(this);
    auto scroll_content = new QWidget(scroll);
    scroll_layout = new QVBoxLayout(scroll_content);
    auto main_layout  = new QVBoxLayout(this);
    main_layout->addWidget(scroll);
    scroll->setWidget(scroll_content);
    scroll->setWidgetResizable(true);
    scroll->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    scroll_layout->addStretch(0);


    // FIXME: small hack to set minimum size of the scrollarea
    QTimer* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [=]() {
        scroll->setMinimumWidth(scroll_layout->sizeHint().width());
    });
    t->start(500);

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &ListContainer::handleNewAC);
    connect(DispatcherUi::get(), &DispatcherUi::ac_deleted, this, &ListContainer::removeAC);
}

void ListContainer::handleNewAC(QString ac_id) {
    auto pageWidget = constructor(ac_id, this);
    widgets[ac_id] = pageWidget;
    scroll_layout->insertWidget(scroll_layout->count()-1, pageWidget);
}

void ListContainer::removeAC(QString ac_id) {
    scroll_layout->removeWidget(widgets[ac_id]);
    widgets[ac_id]->deleteLater();
    widgets.remove(ac_id);
}
