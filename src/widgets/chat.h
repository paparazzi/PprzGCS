#ifndef CHAT_H
#define CHAT_H

#include <QFrame>
#include <QMap>
class QPushButton;
class QToolButton;
class QButtonGroup;
class QHBoxLayout;

namespace Ui {
class Chat;
}

class Chat : public QFrame
{
    Q_OBJECT

public:
    explicit Chat(QWidget *parent = nullptr);
    ~Chat();

private:

    void onSend();
    //void onNewMessage();
    void addMessage(QString txt, QString source, QString dst=QString(), bool sent=false);

    void onNewAircraft(QString ac_id);
    void onAcDeleted(QString ac_id);

    QString chat_id;

    // filter row (with more than one aircraft): All, each aircraft, Server, Mine
    void addFilter(QString category, QString name, QString color);
    void applyFilter();
    void updateDestButton();
    QWidget* filter_row;
    QHBoxLayout* filter_layout;
    QButtonGroup* filter_group;
    QMap<QString, QPushButton*> filters;
    QString current_filter = "all";
    QToolButton* dest_button;

    Ui::Chat *ui;
};

#endif // CHAT_H
