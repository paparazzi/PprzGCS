#include "chatbubble.h"
#include "AircraftManager.h"
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QStyle>

ChatBubble::ChatBubble(QString source, QString dest, QString msg, bool sent, QWidget *parent) :
    QWidget(parent),
    source(source),
    dest(dest)
{
    setObjectName(sent ? "chat_row_sent" : "chat_row");
    setAttribute(Qt::WA_StyledBackground);
    auto h = new QHBoxLayout(this);
    h->setContentsMargins(8, 2, 8, 2);
    h->setSpacing(8);

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
    tag->setMinimumWidth(56);
    tag->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    tag->setStyleSheet(QString("background: %1; color: white; border-radius: 4px; padding: 1px 6px;").arg(color.name()));
    h->addWidget(tag, 0, Qt::AlignTop);

    message = new QLabel(this);
    message->setObjectName("chat_text");
    message->setWordWrap(true);
    message->setTextFormat(Qt::RichText);
    message->setTextInteractionFlags(Qt::TextSelectableByMouse);
    h->addWidget(message, 1);

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
    lines.append(text);
    message->setText(lines.join("<br>"));
    chat_time = QTime::currentTime();
    setToolTip(chat_time.toString("HH:mm:ss"));
    updateLevel();
}

void ChatBubble::updateLevel() {
    if(property("level").toString() != _level) {
        setProperty("level", _level);
        style()->unpolish(this);
        style()->polish(this);
        message->style()->unpolish(message);
        message->style()->polish(message);
    }
}

bool ChatBubble::eventFilter(QObject *obj, QEvent *ev) {
    (void)obj;
    (void)ev;
    return false;
}
