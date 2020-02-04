#ifndef AC_SELECTOR_H
#define AC_SELECTOR_H

#include <QWidget>

namespace Ui {
class Butbut;
}

class ACSelector : public QWidget
{
    Q_OBJECT

public:
    explicit ACSelector(QWidget *parent = nullptr);
    ~ACSelector();

private:
    Ui::Butbut *ui;

private slots:
    void changeAC(int ac_id);

};

#endif // AC_SELECTOR_H
