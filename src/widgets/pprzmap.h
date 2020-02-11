#ifndef PPRZMAP_H
#define PPRZMAP_H

#include <QWidget>

namespace Ui {
class PprzMap;
}

class PprzMap : public QWidget
{
    Q_OBJECT

public:
    explicit PprzMap(QWidget *parent = nullptr);
    ~PprzMap();

protected slots:
    void toggleTileProvider(bool);

private:
    Ui::PprzMap *ui;
};

#endif // PPRZMAP_H
