#include "scriptedit.h"
#include "ui_scriptedit.h"
#include "PprzApplication.h"
#include "PprzToolbox.h"
#include "python_plugins.h"

ScriptEdit::ScriptEdit(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScriptEdit)
{
    ui->setupUi(this);
    connect(ui->run_btn, &QPushButton::clicked, this, &ScriptEdit::handleRun);
}

ScriptEdit::~ScriptEdit()
{
    delete ui;
}

void ScriptEdit::handleRun() {
    auto list = ui->text_edit->toPlainText().trimmed().split("\n");
    pprzApp()->toolbox()->plugins()->runScript(list);
}
