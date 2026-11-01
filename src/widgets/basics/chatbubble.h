#ifndef CHATBUBBLE_H
#define CHATBUBBLE_H

#include <QWidget>
#include <QTime>
#include <QLabel>

/**
 * One chat message: a tag with the full sender name in its aircraft color, and the text, which
 * wraps under itself. The time is the tooltip. Warnings (low, lost, fail, ...) are highlighted.
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

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    void updateLevel();

    QString source;
    QString dest;
    QString _category;
    QTime chat_time;
    QLabel* tag;
    QLabel* message;
    QStringList lines;
    QString _level;
};

#endif // CHATBUBBLE_H
