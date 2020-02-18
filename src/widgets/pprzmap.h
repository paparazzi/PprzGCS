#ifndef PPRZMAP_H
#define PPRZMAP_H

#include <QWidget>
#include <QKeyEvent>

namespace Ui {
class PprzMap;
}

class PprzMap : public QWidget
{
    Q_OBJECT

public:
    explicit PprzMap(QWidget *parent = nullptr);
    ~PprzMap();

protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

protected slots:

private:
    Ui::PprzMap *ui;
    bool drawState;
};

#endif // PPRZMAP_H
