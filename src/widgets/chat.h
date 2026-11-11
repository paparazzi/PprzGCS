#ifndef CHAT_H
#define CHAT_H

#include "flowlayout.h"
#include <QFrame>
#include <QMap>
#include <QHash>
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

signals:
    /// Number of received messages not yet seen (the chat was hidden when they arrived).
    void unreadChanged(int count);

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    /// the message list as tall as its (wrapped) messages at the current width; the chat as tall
    /// as the messages when it opens (not when the user resized its card)
    void fitMessages();

    void onSend();
    //void onNewMessage();
    void addMessage(QString txt, QString source, QString dst=QString(), bool sent=false);

    void onNewAircraft(QString ac_id);
    void onAcDeleted(QString ac_id);

    QString chat_id;
    int unread = 0;
    QHash<QString, qint64> last_errors;     // TELEMETRY_ERROR shown, when

    // filter row (with more than one aircraft): All, each aircraft, Server, Mine
    void addFilter(QString category, QString name, QString color);
    void applyFilter();
    void updateDestButton();
    /// remove the messages shown (those of the current filter)
    void clearMessages();
    void updateClearButton();
    QWidget* filter_row;
    FlowLayout* filter_layout;     // the filters wrap on more lines (many aircraft)
    QList<QPushButton*> filter_order;
    QButtonGroup* filter_group;
    QMap<QString, QPushButton*> filters;
    QString current_filter = "all";
    QToolButton* dest_button;
    QToolButton* clear_button;

    Ui::Chat *ui;
};

#endif // CHAT_H
