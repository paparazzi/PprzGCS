#ifndef PPRZMAIN_H
#define PPRZMAIN_H

#include <QMainWindow>
#include <QtWidgets>

enum LaunchTypes {
    DEFAULT,
    NORMAL,
    FLIGHTPLAN_EDIT,
    CONFIGURE,
    QUIT,
};

class PprzMain : public QMainWindow
{
    Q_OBJECT

public:
    explicit PprzMain(QWidget *parent = nullptr);
    static LaunchTypes launch_type;
    static int const EXIT_CODE_REBOOT = -123456;
    void setupUi(int width, int height, QWidget* centralWidget);

    void setServerStatus(bool active);

signals:
    void ready();


protected:
    // the menu bar is also the title bar (no window frame): move, maximize and resize
    bool eventFilter(QObject* obj, QEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void setupTitleBar();
    Qt::Edges edgesAt(QPoint global_pos);
    QToolButton* max_button = nullptr;
    bool resize_cursor = false;
    void newAC(QString ac_id);
    void removeAC(QString ac_id);
    void delete_ac(QString ac_id);

    void populate_menu();

    QMenuBar *menu_bar;
    QStatusBar *statusBar;

    QMenu* aircraftsTopMenu;
    QMap<QString, QMenu*> acMenus;
    QMap<QString, QAction*> acActions;
    QLabel* serverStatusLed;
    QLabel* linkStatusLed;      // link of the selected aircraft
    QLabel* linkStatusLabel;
    QMap<QString, int> link_states;
    void updateLinkStatus();
};


#endif // PPRZMAIN_H
