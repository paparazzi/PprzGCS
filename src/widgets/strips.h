#ifndef STRIPS_H
#define STRIPS_H

#include <QWidget>
#include <QStackedWidget>

class Strips : public QStackedWidget
{
    Q_OBJECT
public:
    explicit Strips(QWidget *parent = nullptr);

signals:

private:
    void handleNewAC(QString ac_id);

    std::map<QString, int> ac_ids;
public slots:
};

#endif // STRIPS_H
