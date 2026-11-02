#include "listcontainer.h"
#include "dispatcher_ui.h"
#include "AircraftManager.h"
#include "strip.h"
#include <QSplitter>
#include "panels.h"
#include <QTimer>

ListContainer::ListContainer(std::function<QWidget*(QString, QWidget*)> constructor, QWidget *parent) :
    ListContainer(constructor, nullptr, parent)
{}

ListContainer::ListContainer(std::function<QWidget*(QString, QWidget*)> constructor,
                             std::function<QWidget*(QString, QWidget*)> alt_constructor,
                             QWidget *parent) : QScrollArea(parent),
    constructor(constructor),
    alt_constructor(alt_constructor)
{
    auto scroll_content = new QWidget(this);
    setWidget(scroll_content);
    setWidgetResizable(true);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto vbox = new QVBoxLayout(scroll_content);
    grid_layout = new QGridLayout();
    grid_layout->setHorizontalSpacing(4);
    vbox->addLayout(grid_layout);
    vbox->addStretch(1);

    // FIXME: small hack to set minimum size of the scrollarea
    QTimer* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [=,this]() {
        // the content, plus the frame and the vertical scroll bar
        this->setMinimumWidth(widget()->sizeHint().width() + 2 * frameWidth() + verticalScrollBar()->sizeHint().width());
        // the content got taller or shorter (e.g. a strip shows a warning row): fit again
        // or the splitter changed (e.g. a tab group was added) and the strips need a scroll bar
        bool cut = verticalScrollBar()->isVisible() && widget()->sizeHint().height() < 600;
        if(widget()->sizeHint().height() != fitted_height || cut) {
            fitInSplitter();
        }
    });
    t->start(500);

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &ListContainer::handleNewAC);
    // once laid out, take only the height of the content in a vertical splitter
    QTimer::singleShot(0, this, &ListContainer::fitInSplitter);
    connect(PanelManager::get(), &PanelManager::panelsMoved, this, [this]() {
        QTimer::singleShot(0, this, &ListContainer::fitInSplitter);
    });
    connect(DispatcherUi::get(), &DispatcherUi::ac_deleted, this, &ListContainer::removeAC);
    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this, [this](QString) {
        QTimer::singleShot(0, this, &ListContainer::showSelected);
    });
    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this, [this](QString id) {
        selected_ac = id;
        updateCompact();
    });
}

void ListContainer::updateCompact() {
    bool many = widgets.size() > 3;
    for(auto it = widgets.begin(); it != widgets.end(); ++it) {
        bool compact = many && it.key() != selected_ac;
        if(it.value()->metaObject()->indexOfSlot("setCompact(bool)") >= 0) {
            QMetaObject::invokeMethod(it.value(), "setCompact", Q_ARG(bool, compact));
        }
    }
    QTimer::singleShot(0, this, &ListContainer::fitInSplitter);
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
    // a thin bar in the aircraft color (click: select it), and a small menu arrow below
    auto header = new QWidget(this);
    auto header_vbox = new QVBoxLayout(header);
    header_vbox->setContentsMargins(0, 0, 0, 0);
    header_vbox->setSpacing(2);

    auto select_ac_button = new QToolButton(header);
    select_ac_button->setFixedWidth(8);
    select_ac_button->setMinimumHeight(20);
    select_ac_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    select_ac_button->setProperty("ac_color", ac->getColor().name());
    select_ac_button->setToolTip(ac->name());
    header_vbox->addWidget(select_ac_button, 0, Qt::AlignHCenter);
    if(alt_constructor != nullptr) {
        auto action_button = new QToolButton(header);
        action_button->setArrowType(Qt::DownArrow);
        action_button->setAutoRaise(true);
        action_button->setFixedSize(20, 20);
        action_button->setToolTip("Commands");
        header_vbox->addWidget(action_button);
        QWidget* alt_widget = alt_constructor(ac_id, nullptr);
        alt_widgets[ac_id] = alt_widget;
        alt_widget->setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);

        connect(action_button, &QToolButton::clicked, this,
            [=,this]() {
                auto p = action_button->mapToGlobal(action_button->rect().bottomRight());
                alt_widget->move(p);
                alt_widget->show();
                auto screen = action_button->screen();
                if (screen) {
                    auto rect_screen = screen->geometry();
                    if(!rect_screen.contains(alt_widget->geometry())) {
                        // the popup widget does not fit in the screen
                        // move it to be contained in the current screen.
                        auto dp = rect_screen.bottomRight() - alt_widget->geometry().bottomRight();
                        dp = QPoint(min(0, dp.x()), min(0, dp.y()));
                        alt_widget->move(p + dp);
                    }
                }

            });
        connect(select_ac_button, &QObject::destroyed, action_button,
                [=,this]() {
            action_button->deleteLater();
        });
    }

    grid_layout->addWidget(header, row, 0);

    buttons[select_ac_button] = ac_id;

    updateCompact();

    connect(select_ac_button, &QToolButton::clicked, this,
        [=,this]() {
            emit DispatcherUi::get()->ac_selected(ac_id);
        });

    pageWidget->setProperty("ac_id", ac_id);
    pageWidget->installEventFilter(this);
    showSelected();

}

bool ListContainer::eventFilter(QObject* obj, QEvent* event) {
    if(event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        QString ac_id = obj->property("ac_id").toString();
        if(!ac_id.isEmpty() && AircraftManager::get()->aircraftExists(ac_id)) {
            emit DispatcherUi::get()->ac_selected(ac_id);
            auto ac = AircraftManager::get()->getAircraft(ac_id);
            if(event->type() == QEvent::MouseButtonDblClick && ac->isReal()) {
                emit DispatcherUi::get()->centerMap(ac->getPosition());
            }
        }
    }
    return QScrollArea::eventFilter(obj, event);
}

/// The bar of the selected aircraft solid and wider, the others faded.
void ListContainer::showSelected() {
    QString selected = DispatcherUi::get()->getSelectedAcId();
    for(auto it = buttons.begin(); it != buttons.end(); ++it) {
        auto b = qobject_cast<QToolButton*>(it.key());
        if(b == nullptr) {
            continue;
        }
        bool sel = it.value() == selected;
        QColor c(b->property("ac_color").toString());
        if(!sel) {
            c.setAlpha(110);
        }
        b->setFixedWidth(sel ? 10 : 6);
        b->setStyleSheet(QString("QToolButton {background: %1; border: 0; border-radius: 3px;}").arg(c.name(QColor::HexArgb)));
    }
}

void ListContainer::removeAC(QString ac_id) {
    grid_layout->removeWidget(widgets[ac_id]);
    widgets[ac_id]->deleteLater();
    widgets.remove(ac_id);

    if(alt_widgets.contains(ac_id)) {
        alt_widgets[ac_id]->deleteLater();
        alt_widgets.remove(ac_id);
    }

    auto rect = std::find(buttons.begin(), buttons.end(), ac_id);
    if(rect != buttons.end()) {
        grid_layout->removeWidget(rect.key());
        rect.key()->deleteLater();
        buttons.remove(rect.key());
    }

    updateCompact();
}

/**
 * In a vertical splitter (e.g. the strips above the flight plan), take the height of the
 * content (at most 60 % of the splitter) and give the rest to the other widgets.
 */
void ListContainer::fitInSplitter() {
    auto splitter = qobject_cast<QSplitter*>(parentWidget());
    if(splitter == nullptr || splitter->orientation() != Qt::Vertical || splitter->count() < 2) {
        return;
    }
    int idx = splitter->indexOf(this);
    auto sizes = splitter->sizes();
    int total = 0;
    for(auto s: sizes) {
        total += s;
    }
    if(idx < 0 || total <= 0) {
        return;
    }
    fitted_height = widget()->sizeHint().height();
    int content = fitted_height + 2 * frameWidth();
    int wanted = qBound(minimumSizeHint().height(), content, total * 6 / 10);
    int others = total - sizes[idx];
    int rest = total - wanted;
    for(int i=0; i<sizes.size(); i++) {
        if(i == idx) {
            sizes[i] = wanted;
        } else {
            sizes[i] = others > 0 ? sizes[i] * rest / others : rest / (sizes.size() - 1);
        }
    }
    splitter->setSizes(sizes);
}
