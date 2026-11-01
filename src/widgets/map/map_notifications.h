#ifndef MAP_NOTIFICATIONS_H
#define MAP_NOTIFICATIONS_H

#include <QtWidgets>
#include <functional>

/**
 * Notifications at the top of the map: chat messages (gone after a few seconds) and warnings
 * or alarms such as a low battery or a lost link (until fixed or dismissed). At most three rows;
 * alarms first; repeated messages of one sender update one row with a counter; the rest is
 * counted in a button that opens the chat.
 */
class MapNotifications : public QWidget
{
    Q_OBJECT
public:
    enum Level {INFO = 0, WARNING = 1, ALARM = 2};

    explicit MapNotifications(QWidget *parent = nullptr);

    /// A message (key: the sender, to merge repeats)
    void message(QString sender, QString text);
    /// A warning or alarm that stays until cleared (key: e.g. "bat:5"); level INFO clears it
    void setState(QString key, QString sender, QString text, Level level);

    void reposition();

signals:
    void openChat();
    void senderClicked(QString sender);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    struct Item {
        QString key;
        QString sender;
        QString text;
        Level level;
        int count = 1;
        QTime time;
        QDateTime expires;      // invalid for states
        bool dismissed = false;
        Level dismissed_level = INFO;
        bool hovered = false;
    };

    void rebuild();
    QWidget* makeRow(const Item& item);
    void expire();
    static QString senderName(QString sender, QColor* color);

    QList<Item> items;          // newest last
    QVBoxLayout* layout;
    QPushButton* more_button;
    QTimer* timer;
    QList<QDateTime> recent;    // arrival times of recent messages (busy: shorter time)
    int hidden_messages = 0;
};

#endif // MAP_NOTIFICATIONS_H
