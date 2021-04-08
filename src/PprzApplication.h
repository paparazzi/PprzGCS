#ifndef PPRZAPPLICATION_H
#define PPRZAPPLICATION_H

#include <QApplication>

class PprzToolbox;

class PprzApplication : public QApplication
{
    Q_OBJECT
public:
    explicit PprzApplication(int &argc, char **argv, int = ApplicationFlags);

    static PprzApplication*  _app;
    void init();

    PprzToolbox* toolbox() {return _toolbox;}

private:
    PprzToolbox* _toolbox = nullptr;

signals:

};

/// @brief Returns the PprzApplication object singleton.
PprzApplication* pprzApp(void);

#endif // PPRZAPPLICATION_H
