#include "pprzmain.h"
#include <QMessageBox>
#include <QDebug>
#include "dispatcher_ui.h"
#include "AircraftManager.h"
#include "pprz_dispatcher.h"
#include <QSettings>
#include "app_settings.h"
#include "gcs_utils.h"
#include <QColorDialog>
#include "panels.h"

#if defined(SPEECH_ENABLED)
#include "speaker.h"
#endif

LaunchTypes PprzMain::launch_type = DEFAULT;

PprzMain::PprzMain(QWidget *parent) :
    QMainWindow(parent)
{

}

void PprzMain::setupUi(int width, int height, QWidget* centralWidget) {
    centralWidget->setParent(this);
    resize(width, height);
    menu_bar = new QMenuBar(this);
    menu_bar->setObjectName(QString::fromUtf8("menuBar"));
    menu_bar->setGeometry(QRect(0, 0, 555, 22));
    setMenuBar(menu_bar);
    statusBar = new QStatusBar(this);
    statusBar->setObjectName(QString::fromUtf8("statusBar"));
    setStatusBar(statusBar);
    setCentralWidget(centralWidget);
    setWindowIcon(QIcon(":/pictures/icon.svg"));
    populate_menu();
    // the menu bar as the title bar: an option, as the window then has no border or shadow from the
    // window manager (e.g. KDE: its edges are hard to see)
    if(getAppSettings().value("window/menu_title_bar", false).toBool()) {
        setupTitleBar();
    }

    // on the right: the server, and the link of the selected aircraft
    serverStatusLed = new QLabel(statusBar);
    setServerStatus(false);
    statusBar->addPermanentWidget(serverStatusLed);
    statusBar->addPermanentWidget(new QLabel("Server  "));
    linkStatusLed = new QLabel(statusBar);
    linkStatusLabel = new QLabel(statusBar);
    statusBar->addPermanentWidget(linkStatusLed);
    statusBar->addPermanentWidget(linkStatusLabel);
    updateLinkStatus();
    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this, &PprzMain::updateLinkStatus);

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &PprzMain::newAC);
    connect(DispatcherUi::get(), &DispatcherUi::ac_deleted, this, &PprzMain::removeAC);

    emit ready();
}

void PprzMain::setServerStatus(bool active) {
    QIcon ic;
    if(active) {
        ic = QIcon(":/pictures/green_led.svg");
    } else {
        ic = QIcon(":/pictures/red_led.svg");
    }
    serverStatusLed->setPixmap(ic.pixmap(15, 15));
}

void PprzMain::populate_menu() {

    auto file_menu = menu_bar->addMenu("&File");

    auto user_dir = file_menu->addAction("Open user directory");
    connect(user_dir, &QAction::triggered, [=,this](){
        auto settings = getAppSettings();
        QString path = QDir::toNativeSeparators(appConfig()->value("USER_DATA_PATH").toString());
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });

    auto app_dir = file_menu->addAction("Open app directory");
    connect(app_dir, &QAction::triggered, [=,this](){
        auto settings = getAppSettings();
        QString path = QDir::toNativeSeparators(appConfig()->value("APP_DATA_PATH").toString());
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });

    auto edit_settings = file_menu->addAction("Edit Settings");
    connect(edit_settings, &QAction::triggered, [=,this](){
        auto se = new SettingsEditor();
        se->open();
    });

    auto silent_mode_action = file_menu->addAction("Silent mode");
    silent_mode_action->setCheckable(true);
    silent_mode_action->setChecked(PprzDispatcher::get()->isSilent());

    auto speech_action = file_menu->addAction("Enable Speech");
    speech_action->setCheckable(true);
    speech_action->setChecked(speech());

    auto map_track_ac = file_menu->addAction("Track AC");
    map_track_ac->setCheckable(true);
    map_track_ac->setChecked(false);

    auto open_flight_plan = file_menu->addAction("Open FlightPlans");
    connect(open_flight_plan, &QAction::triggered, [=,this](){
        auto settings = getAppSettings();
        auto pprz_home = appConfig()->value("PAPARAZZI_HOME").toString();
        auto files = QFileDialog::getOpenFileNames(this, "open fp", pprz_home + "/conf/flight_plans", "*.xml");
        if(PprzMain::launch_type != FLIGHTPLAN_EDIT) {
            PprzMain::launch_type = FLIGHTPLAN_EDIT;
            appConfig()->setValue("FLIGHTPLAN_FILES", files);
            qApp->exit( PprzMain::EXIT_CODE_REBOOT );
        } else {
            for(auto &fp_file: files) {
                qDebug() << "edit flightplan " << fp_file;
                auto name = fp_file.split("/").last();
                AircraftManager::get()->addFPAircraft(name, fp_file);
            }
        }
    });

    connect(silent_mode_action, &QAction::toggled, [=](bool checked) {
        PprzDispatcher::get()->setSilent(checked);
    });
    // also left from the banner on the map
    connect(PprzDispatcher::get(), &PprzDispatcher::silentChanged, silent_mode_action, &QAction::setChecked);

#if defined(SPEECH_ENABLED)
    connect(speech_action, &QAction::toggled, pprzApp()->toolbox()->speaker(), &Speaker::enableSpeech);
#endif

    // follow the aircraft, heading up (also the follow button of the map)
    connect(map_track_ac, &QAction::triggered, [=](bool checked) {
        emit DispatcherUi::get()->followAircraft(checked ? 2 : 0);
    });
    connect(DispatcherUi::get(), &DispatcherUi::followAircraft, map_track_ac, [=](int mode) {
        map_track_ac->setChecked(mode == 2);
    });

    auto reset_panels = file_menu->addAction("Reset panels");
    reset_panels->setToolTip("Back to the panels of the layout file");
    connect(reset_panels, &QAction::triggered, this, [](){
        PanelManager::get()->reset();
    });

    auto quit = file_menu->addAction("&Quit");
    connect(quit, &QAction::triggered, qApp, QApplication::quit);


    aircraftsTopMenu = menu_bar->addMenu("&Aircrafts");

    aircraftsTopMenu->addAction("Update", PprzDispatcher::get(), &PprzDispatcher::requestAircrafts);

    // the color of each aircraft (saved by aircraft name)
    auto colors_menu = aircraftsTopMenu->addMenu("Colors");
    connect(colors_menu, &QMenu::aboutToShow, this, [=,this]() {
        colors_menu->clear();
        for(auto ac: AircraftManager::get()->getAircrafts()) {
            if(!ac->isReal()) {
                continue;
            }
            QPixmap swatch(14, 14);
            swatch.fill(ac->getColor());
            QString id = ac->getId();
            QString name = ac->name();
            colors_menu->addAction(QIcon(swatch), name + "\u2026", this, [=,this]() {
                auto color = QColorDialog::getColor(AircraftManager::get()->getAircraft(id)->getColor(),
                                                    this, "Color of " + name);
                if(color.isValid()) {
                    auto settings = getAppSettings();
                    settings.setValue("aircraft_colors/" + name, color.name());
                    PprzDispatcher::get()->reloadAircraft(id);
                }
            });
        }
        colors_menu->addSeparator();
        colors_menu->addAction("Back to the configured colors", this, [=]() {
            auto settings = getAppSettings();
            settings.remove("aircraft_colors");
            for(auto ac: AircraftManager::get()->getAircrafts()) {
                if(ac->isReal()) {
                    PprzDispatcher::get()->reloadAircraft(ac->getId());
                }
            }
        });
    });

    auto help_menu = menu_bar->addMenu("&Help");
    auto about = help_menu->addAction("&About");

    QString about_txt =  QString(
            "<h1>PprzGCS</h1>"
            "version %1"
            "<p>PprzGCS is a ground control software for <a href=\"https://paparazziuav.org\">paparrazi UAV</a> drones.</p>"
            "<p>source code can be found here: "
            "<a href=\"https://github.com/paparazzi/PprzGCS\">https://github.com/paparazzi/PprzGCS</a></p>"
            "<p>Copyright (C) 2019-2022 Fabien Bonneval (fabien.bonneval@gmail.com)</p>"
            "<p>PprzGCS is free software; you can redistribute it and/or modify "
            "it under the terms of the <a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">GNU General Public License</a> as published by "
            "the Free Software Foundation; either version 3, or (at your option) "
            "any later version.</p>"
            "<p>The program is provided AS IS with "
            "NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, "
            "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.</p>"
            "<h2>Third-party libraries</h2>"
            "<ul>"
            "<li><a href=\"https://www.qt.io/\">Qt6</a></li>"
            "<li><a href=\"https://proj.org/\">Proj</a></li>"
            "<li><a %1 href=\"https://libzip.org/\">libzip</a></li>"
            "</ul>").arg(QCoreApplication::applicationVersion());

    connect(about, &QAction::triggered, [=,this]() {
        QMessageBox::about(this,"About PprzGCS", about_txt);

    });

    auto doc = help_menu->addAction("&Documentation");

    connect(doc, &QAction::triggered, [=]() {
        QDesktopServices::openUrl(QUrl("https://docs.paparazziuav.org/PprzGCS/"));
    });

}


void PprzMain::newAC(QString ac_id) {
    QString acName = AircraftManager::get()->getAircraft(ac_id)->name();
    auto menu = new QMenu("&" + acName);
    auto action = aircraftsTopMenu->addMenu(menu);

    acMenus[ac_id] = menu;
    acActions[ac_id] = action;

    menu->addAction("Remove", this, [ac_id](){
        AircraftManager::get()->removeAircraft(ac_id);
    });

    auto watcher = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getWatcher();
    connect(watcher, &AircraftWatcher::link_status, menu, [=,this](AircraftWatcher::LinkStatus status) {
        link_states[ac_id] = static_cast<int>(status);
        if(DispatcherUi::get()->getSelectedAcId() == ac_id) {
            updateLinkStatus();
        }
    });
}

void PprzMain::updateLinkStatus() {
    auto ac_id = DispatcherUi::get()->getSelectedAcId();
    if(ac_id.isEmpty() || !link_states.contains(ac_id)) {
        linkStatusLed->clear();
        linkStatusLabel->setText(ac_id.isEmpty() ? "" : "Link: no data");
        return;
    }
    auto status = static_cast<AircraftWatcher::LinkStatus>(link_states[ac_id]);
    QString led = status == AircraftWatcher::LINK_OK ? ":/pictures/green_led.svg" : ":/pictures/red_led.svg";
    linkStatusLed->setPixmap(QIcon(led).pixmap(15, 15));
    switch(status) {
    case AircraftWatcher::LINK_OK:
        linkStatusLabel->setText("Link");
        break;
    case AircraftWatcher::LINK_PARTIALY_LOST:
        linkStatusLabel->setText("Link: partly lost");
        break;
    case AircraftWatcher::LINK_LOST:
        linkStatusLabel->setText("Link lost");
        break;
    }
}

void PprzMain::removeAC(QString ac_id) {
    auto action = acActions[ac_id];
    auto menu = acMenus[ac_id];

    acActions.remove(ac_id);
    acMenus.remove(ac_id);
    link_states.remove(ac_id);

    aircraftsTopMenu->removeAction(action);

    menu->deleteLater();
    action->deleteLater();

}


/**
 * No window frame: the menu bar is the title bar (drag its empty part to move the window, double
 * click to maximize), with minimize, maximize and close on the right. The edges resize the window.
 * Saves the height of the title bar, and follows the theme (also the night one).
 */
void PprzMain::setupTitleBar() {
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    auto corner = new QWidget(menu_bar);
    auto lay = new QHBoxLayout(corner);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    auto make = [=,this](QString text, QString tip, QString name) {
        auto b = new QToolButton(corner);
        b->setText(text);
        b->setToolTip(tip);
        b->setObjectName(name);
        b->setAutoRaise(true);
        b->setFixedSize(40, 26);
        lay->addWidget(b);
        return b;
    };
    auto min_b = make(QString::fromUtf8("\u2013"), "Minimize", "window_button");
    max_button = make(QString::fromUtf8("\u25A1"), "Maximize", "window_button");
    auto close_b = make(QString::fromUtf8("\u2715"), "Close", "window_close");
    connect(min_b, &QToolButton::clicked, this, &QWidget::showMinimized);
    connect(max_button, &QToolButton::clicked, this, [this]() {
        isMaximized() ? showNormal() : showMaximized();
    });
    connect(close_b, &QToolButton::clicked, this, &QWidget::close);
    if(!qApp->styleSheet().contains("window_close")) {
        corner->setStyleSheet("QToolButton {border: 0; font-size: 14px;}"
                              "QToolButton:hover {background: rgba(128, 128, 128, 60);}"
                              "QToolButton#window_close:hover {background: #d92d20; color: white;}");
    }
    menu_bar->setCornerWidget(corner, Qt::TopRightCorner);
    menu_bar->installEventFilter(this);
    qApp->installEventFilter(this);     // the edges of the window: resize
}

Qt::Edges PprzMain::edgesAt(QPoint global_pos) {
    Qt::Edges edges;
    if(isMaximized() || isFullScreen()) {
        return edges;
    }
    const int m = 6;
    QRect r = frameGeometry();
    if(!r.adjusted(-1, -1, 1, 1).contains(global_pos)) {
        return edges;
    }
    if(global_pos.x() <= r.left() + m) edges |= Qt::LeftEdge;
    if(global_pos.x() >= r.right() - m) edges |= Qt::RightEdge;
    if(global_pos.y() <= r.top() + m) edges |= Qt::TopEdge;
    if(global_pos.y() >= r.bottom() - m) edges |= Qt::BottomEdge;
    return edges;
}

bool PprzMain::eventFilter(QObject* obj, QEvent* event) {
    auto w = qobject_cast<QWidget*>(obj);
    bool in_this_window = w != nullptr && w->window() == this;
    if(in_this_window && (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress)) {
        auto me = static_cast<QMouseEvent*>(event);
        auto edges = edgesAt(me->globalPosition().toPoint());
        if(event->type() == QEvent::MouseMove && me->buttons() == Qt::NoButton) {
            // the resize cursor along the edges
            Qt::CursorShape shape = Qt::ArrowCursor;
            if((edges & Qt::LeftEdge && edges & Qt::TopEdge) || (edges & Qt::RightEdge && edges & Qt::BottomEdge)) {
                shape = Qt::SizeFDiagCursor;
            } else if((edges & Qt::RightEdge && edges & Qt::TopEdge) || (edges & Qt::LeftEdge && edges & Qt::BottomEdge)) {
                shape = Qt::SizeBDiagCursor;
            } else if(edges & (Qt::LeftEdge | Qt::RightEdge)) {
                shape = Qt::SizeHorCursor;
            } else if(edges & (Qt::TopEdge | Qt::BottomEdge)) {
                shape = Qt::SizeVerCursor;
            }
            if(edges && !resize_cursor) {
                QApplication::setOverrideCursor(shape);
                resize_cursor = true;
            } else if(edges && resize_cursor) {
                QApplication::changeOverrideCursor(shape);
            } else if(!edges && resize_cursor) {
                QApplication::restoreOverrideCursor();
                resize_cursor = false;
            }
        } else if(event->type() == QEvent::MouseButtonPress && me->button() == Qt::LeftButton && edges &&
                  windowHandle() != nullptr && windowHandle()->startSystemResize(edges)) {
            return true;
        }
    }
    if(obj == menu_bar) {
        // the empty part of the menu bar: move the window, double click to maximize
        if(event->type() == QEvent::MouseButtonPress) {
            auto me = static_cast<QMouseEvent*>(event);
            if(me->button() == Qt::LeftButton && menu_bar->actionAt(me->pos()) == nullptr &&
               edgesAt(me->globalPosition().toPoint()) == Qt::Edges() &&
               windowHandle() != nullptr && windowHandle()->startSystemMove()) {
                return true;
            }
        } else if(event->type() == QEvent::MouseButtonDblClick) {
            auto me = static_cast<QMouseEvent*>(event);
            if(menu_bar->actionAt(me->pos()) == nullptr) {
                isMaximized() ? showNormal() : showMaximized();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void PprzMain::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    if(event->type() == QEvent::WindowStateChange && max_button != nullptr) {
        max_button->setText(isMaximized() ? QString::fromUtf8("\u2752") : QString::fromUtf8("\u25A1"));
        max_button->setToolTip(isMaximized() ? "Restore" : "Maximize");
    }
}
