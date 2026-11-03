#ifndef CHATBUBBLE_H
#define CHATBUBBLE_H

#include <QWidget>
#include <QTime>
#include <QLabel>
#include <QVBoxLayout>

/**
 * The messages of one sender in a row: a tag with the name in its aircraft color, the messages
 * below it, a line each. The time is the tooltip. Warnings (low, lost, fail, ...) are tinted lines.
 */
class ChatBubble : public QWidget
{
    Q_OBJECT

public:
    explicit ChatBubble(QString source, QString dest, QString msg, bool sent=false, QWidget *parent = nullptr);

    QString getSource() {return source;}
    QString getDest() {return dest;}
    QTime getTime() {return chat_time;}
    /// For the filter: the aircraft id, "server" or "me"
    QString category() {return _category;}
    void addText(QString text);
    /// "alarm", "warning" or "" from the words in the text
    static QString level(QString text);
    QString currentLevel() {return _level;}

private:
    void updateLevel();

    QString source;
    QString dest;
    QString _category;
    QTime chat_time;
    QLabel* tag;
    QVBoxLayout* lines_layout;
    QString _level;
};

#endif // CHATBUBBLE_H
