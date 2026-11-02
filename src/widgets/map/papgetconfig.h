#ifndef PAPGETCONFIG_H
#define PAPGETCONFIG_H

#include <QtWidgets>
#include "papget.h"
#include "card_dialog.h"

/**
 * The look of a papget: size and color, shown on the map while changing them. Apply, Cancel
 * (back to how it was), or Remove the papget.
 */
class PapgetConfig : public CardDialog
{
    Q_OBJECT
public:
    explicit PapgetConfig(Papget::DataDef datadef, Papget::Params params, QWidget *parent = nullptr);

signals:
    void paramsChanged(Papget::Params);
    void removeRequested();

private:
    Papget::DataDef datadef;
    Papget::Params params;
};


#endif // PAPGETCONFIG_H
