#include "chatbubble.h"
#include "AircraftManager.h"
#include <QVBoxLayout>
#include <QFrame>
#include <QRegularExpression>
#include <QStyle>

ChatBubble::ChatBubble(QString source, QString dest, QString msg, bool sent, QWidget *parent) :
    QWidget(parent),
    source(source),
    dest(dest)
{
    setObjectName(sent ? "chat_row_sent" : "chat_row");
    setAttribute(Qt::WA_StyledBackground);
    // the name once, the messages below it at full width with a bar in the aircraft color
    auto v = new QVBoxLayout(this);
    v->setContentsMargins(6, 4, 6, 2);
    v->setSpacing(2);

    QString name = source;
    QColor color(0x6e7781);
    if(sent) {
        name = "Me";
        color = QColor(0x1d5fc9);
        _category = "me";
    } else if(AircraftManager::get()->aircraftExists(source)) {
        auto ac = AircraftManager::get()->getAircraft(source);
        name = ac->name();
        color = ac->getColor();
        _category = source;
    } else {
        name = (source.isEmpty() || source == "ground") ? QString("Server") : source.left(1).toUpper() + source.mid(1);
        _category = "server";
    }
    if(sent && dest != "") {
        name += QString::fromUtf8(" → ") + dest;
    }

    tag = new QLabel(this);
    QFont f = tag->font();
    f.setPixelSize(11);
    f.setBold(true);
    tag->setFont(f);
    tag->setText(QFontMetrics(f).elidedText(name, Qt::ElideRight, 100));
    tag->setToolTip(name);
    tag->setAlignment(Qt::AlignCenter);
    tag->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    tag->setStyleSheet(QString("background: %1; color: white; border-radius: 4px; padding: 1px 6px;").arg(color.name()));
    v->addWidget(tag, 0, Qt::AlignLeft);

    auto body = new QFrame(this);
    body->setObjectName("chat_body");
    body->setStyleSheet(QString("#chat_body {border: 0; border-left: 3px solid %1;}").arg(color.name()));
    lines_layout = new QVBoxLayout(body);
    lines_layout->setContentsMargins(5, 0, 0, 0);
    lines_layout->setSpacing(0);
    v->addWidget(body);

    chat_time = QTime::currentTime();
    addText(msg);
}

/// "alarm", "warning" or "" from the words in the text
QString ChatBubble::level(QString text) {
    static const QRegularExpression alarm("\\b(lost|fail\\w*|error|critical|crash\\w*|kill\\w*|emergency)\\b",
                                          QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression warning("\\b(low|warning|reboot\\w*|mismatch|timeout|degraded)\\b",
                                            QRegularExpression::CaseInsensitiveOption);
    if(alarm.match(text).hasMatch()) {
        return "alarm";
    }
    if(warning.match(text).hasMatch()) {
        return "warning";
    }
    return "";
}

void ChatBubble::addText(QString text) {
    QString lvl = level(text);
    if(lvl == "alarm" || (lvl == "warning" && _level != "alarm")) {
        _level = lvl;
    }
    // bold with *text*
    text = text.toHtmlEscaped();
    static const QRegularExpression re(QStringLiteral("\\*([^*]+)\\*"));
    text.replace(re, "<b>\\1</b>");
    // a line per message: a warning or error is tinted on its own line
    auto message = new QLabel(text, this);
    message->setObjectName("chat_text");
    message->setWordWrap(true);
    message->setTextFormat(Qt::RichText);
    message->setTextInteractionFlags(Qt::TextSelectableByMouse);
    message->setProperty("level", lvl);
    chat_time = QTime::currentTime();
    message->setToolTip(chat_time.toString("HH:mm:ss"));
    lines_layout->addWidget(message);
    updateLevel();
}

void ChatBubble::updateLevel() {
    if(property("level").toString() != _level) {
        setProperty("level", _level);
        style()->unpolish(this);
        style()->polish(this);
    }
}
