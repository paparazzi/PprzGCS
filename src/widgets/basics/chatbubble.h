#ifndef CHATBUBBLE_H
#define CHATBUBBLE_H

#include <QWidget>

namespace Ui {
class ChatBubble;
}

class ChatBubble : public QWidget
{
    Q_OBJECT

public:
    explicit ChatBubble(QString sender, QString dest, QString msg, bool sent=false, QWidget *parent = nullptr);
    ~ChatBubble();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    Ui::ChatBubble *ui;
};

#endif // CHATBUBBLE_H
