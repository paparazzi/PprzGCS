#ifndef SCRIPTEDIT_H
#define SCRIPTEDIT_H

#include <QWidget>

namespace Ui {
class ScriptEdit;
}

class ScriptEdit : public QWidget
{
    Q_OBJECT

public:
    explicit ScriptEdit(QWidget *parent = nullptr);
    ~ScriptEdit();

private:
    void handleRun();

    Ui::ScriptEdit *ui;
};

#endif // SCRIPTEDIT_H
