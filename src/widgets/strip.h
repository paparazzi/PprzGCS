#ifndef STRIP_H
#define STRIP_H

#include <QWidget>

namespace Ui {
class Strip;
}

class Strip : public QWidget
{
    Q_OBJECT

public:
    explicit Strip(QWidget *parent = nullptr);
    ~Strip();

private:
    Ui::Strip *ui;

private slots:
    void changeColor(int ac_id);
};

#endif // STRIP_H
