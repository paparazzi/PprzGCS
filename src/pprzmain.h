#ifndef PPRZMAIN_H
#define PPRZMAIN_H

#include <QMainWindow>
#include <QtWidgets>

class PprzMain : public QMainWindow
{
    Q_OBJECT

public:
    explicit PprzMain(QWidget *parent = nullptr);
    static int const EXIT_CODE_REBOOT = -123456;
    void setupUi(int width, int height, QWidget* centralWidget);

    void setServerStatus(bool active);


private:
    void newAC(QString ac_id);
    void removeAC(QString ac_id);
    void delete_ac(QString ac_id);

    void populate_menu();

    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    QMenu* aircraftsTopMenu;
    QMap<QString, QMenu*> acMenus;
    QMap<QString, QAction*> acActions;
    QLabel* serverStatusLed;

};

#endif // PPRZMAIN_H
