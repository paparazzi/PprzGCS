#include "widget_stack.h"
#include "dispatcher_ui.h"
#include "AircraftManager.h"


WidgetStack::WidgetStack(std::function<QWidget*(QString, QWidget*)> constructor, QWidget *parent, bool headers) : QWidget(parent),
    constructor(constructor)
{
    vLayout = new QVBoxLayout(this);
    vLayout->setSpacing(0);
    auto contentWidget = new QWidget(this);
    stackLayout = new QVBoxLayout(contentWidget);
    contentWidget->setAutoFillBackground(true);
    stackLayout->setContentsMargins(0,0,0,0);
    if(headers) {
        ac_selector = new ACSelector(this);
        vLayout->addWidget(ac_selector);
    }
    vLayout->addWidget(contentWidget);

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &WidgetStack::handleNewAC);
    connect(DispatcherUi::get(), &DispatcherUi::ac_deleted, this, &WidgetStack::removeAC);
    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this,
            [=](QString ac_id) {
                for(auto id : viewers_widgets.keys()) {
                    if(id == ac_id) {
                        viewers_widgets[id]->show();
                    } else {
                        viewers_widgets[id]->hide();
                    }
                }
            });

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void WidgetStack::handleNewAC(QString ac_id) {
    auto sv = constructor(ac_id, this);
    stackLayout->addWidget(sv);
    viewers_widgets[ac_id] = sv;
    sv->hide();
}

void WidgetStack::removeAC(QString ac_id) {
    (void)ac_id;
    auto w = viewers_widgets[ac_id];
    viewers_widgets.remove(ac_id);
    stackLayout->removeWidget(w);
    //viewers_widgets[ac_id]->disconnect();
    w->deleteLater();
}
