#pragma once

#include <QObject>
#include "PprzToolbox.h"
#include "PprzApplication.h"
#include "pprz_dispatcher.h"
#include "pprzmain.h"

class PythonPlugins : public PprzTool
{
    Q_OBJECT
public:
    explicit PythonPlugins(PprzApplication* app, PprzToolbox* toolbox);
    void setToolbox(PprzToolbox* toolbox) override;

    void bind_main_window(PprzMain* w);

signals:

public Q_SLOTS:
    void printDiagnostics();
    void runScript(const QStringList &);

private:


};
