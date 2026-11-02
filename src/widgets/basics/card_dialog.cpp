#include "card_dialog.h"

CardDialog::CardDialog(QString title, QString secondary, QWidget *parent, QColor marker) : QDialog(parent),
    placed(false)
{
    setWindowTitle(title);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setObjectName("card_dialog");

    auto lay = new QVBoxLayout(this);
    lay->setContentsMargins(1, 1, 1, 1);
    lay->setSpacing(0);

    if(!title.isEmpty()) {
        auto header = new QWidget(this);
        header->setObjectName("card_header");
        header->setCursor(Qt::SizeAllCursor);   // drag here to move the card
        header->setAttribute(Qt::WA_StyledBackground);
        auto hl = new QHBoxLayout(header);
        hl->setContentsMargins(12, 8, 12, 8);
        if(marker.isValid()) {
            auto diamond = new QLabel(QString::fromUtf8("◆"), header);
            diamond->setStyleSheet("color: " + marker.name() + "; font-size: 14px;");
            hl->addWidget(diamond);
        }
        auto title_label = new QLabel(title, header);
        title_label->setObjectName("card_title");
        hl->addWidget(title_label, 1);
        if(!secondary.isEmpty()) {
            auto sec = new QLabel(secondary, header);
            sec->setObjectName("card_secondary");
            hl->addWidget(sec);
        }
        lay->addWidget(header);
    }

    auto body = new QWidget(this);
    body_layout = new QVBoxLayout(body);
    body_layout->setContentsMargins(12, 10, 12, 4);
    body_layout->setSpacing(8);
    lay->addWidget(body);

    buttons_layout = new QHBoxLayout();
    buttons_layout->setContentsMargins(12, 6, 12, 12);
    buttons_layout->setSpacing(8);
    lay->addLayout(buttons_layout);

    // colors when the application style sheet does not have them
    if(!qApp->styleSheet().contains("#card_header {")) {
        setStyleSheet(
            "#card_header {background: #f4f5f7; border-bottom: 1px solid #c4ccd5;"
            " border-top-left-radius: 10px; border-top-right-radius: 10px;}"
            "#card_title, #card_question {font-weight: bold;}"
            "#card_secondary, #card_muted {color: #57606a;}"
            "#card_note {background: #fff4e0; color: #8a4b00; border-radius: 6px; padding: 6px 8px;}"
            "QPushButton#card_button, QPushButton#card_button_danger_outline {background: #f4f5f7;"
            " border: 1px solid #c4ccd5; border-radius: 6px; min-height: 32px; padding: 0 12px;}"
            "QPushButton#card_button_danger_outline {color: #b42318;}"
            "QPushButton#card_button_primary {background: #1d5fc9; color: white; font-weight: bold; border: 0;"
            " border-radius: 6px; min-height: 34px; padding: 0 12px;}"
            "QPushButton#card_button_danger {background: #b42318; color: white; font-weight: bold; border: 0;"
            " border-radius: 6px; min-height: 34px; padding: 0 12px;}");
    }
}

QPushButton* CardDialog::addButton(QString text, ButtonKind kind, int stretch) {
    auto b = new QPushButton(text, this);
    switch (kind) {
    case PRIMARY:
        b->setObjectName("card_button_primary");
        break;
    case DANGER:
        b->setObjectName("card_button_danger");
        break;
    case DANGER_OUTLINE:
        b->setObjectName("card_button_danger_outline");
        break;
    default:
        b->setObjectName("card_button");
        break;
    }
    // Enter: the main action only
    b->setAutoDefault(kind == PRIMARY || kind == DANGER);
    b->setDefault(kind == PRIMARY || kind == DANGER);
    buttons_layout->addWidget(b, stretch);
    return b;
}

QLabel* CardDialog::addText(QString text, QString object_name) {
    auto l = new QLabel(text, this);
    l->setObjectName(object_name);
    l->setWordWrap(true);
    body_layout->addWidget(l);
    return l;
}

void CardDialog::paintEvent(QPaintEvent* event) {
    (void)event;
    // the card: rounded, with a border
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(palette().color(QPalette::Mid), 1));
    p.setBrush(palette().color(QPalette::Window));
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 10, 10);
}

void CardDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    if(placed) {
        return;
    }
    // centered on the window it belongs to
    adjustSize();
    QWidget* w = parentWidget() ? parentWidget()->window() : QApplication::activeWindow();
    QRect r;
    if(w != nullptr && w != this) {
        r = w->frameGeometry();
    } else if(auto screen = QGuiApplication::primaryScreen()) {
        r = screen->availableGeometry();
    }
    if(r.isValid()) {
        move(r.center() - QPoint(width() / 2, height() / 2));
    }
    placed = true;
}

// Moving the card by dragging it
void CardDialog::mousePressEvent(QMouseEvent* event) {
    if(event->button() != Qt::LeftButton) {
        QDialog::mousePressEvent(event);
        return;
    }
    // Wayland: only the compositor can move a window. X11: move it here (GNOME does not move
    // modal dialogs with the window manager move)
    if(QGuiApplication::platformName().startsWith("wayland") &&
       windowHandle() != nullptr && windowHandle()->startSystemMove()) {
        event->accept();
        return;
    }
    dragging = true;
    drag_offset = event->globalPosition().toPoint() - frameGeometry().topLeft();
    event->accept();
}

void CardDialog::mouseMoveEvent(QMouseEvent* event) {
    if(dragging) {
        move(event->globalPosition().toPoint() - drag_offset);
        event->accept();
    } else {
        QDialog::mouseMoveEvent(event);
    }
}

void CardDialog::mouseReleaseEvent(QMouseEvent* event) {
    dragging = false;
    QDialog::mouseReleaseEvent(event);
}

/// To the right of the point (and of `avoid`), or on the left when there is no room; on the screen.
void CardDialog::placeNear(QPoint global_pos, QRect avoid) {
    adjustSize();
    QRect keep = avoid.united(QRect(global_pos, QSize(1, 1))).adjusted(-28, -28, 28, 28);
    QPoint pos(keep.right(), global_pos.y() - height() / 3);
    auto screen = QGuiApplication::screenAt(global_pos);
    if(screen) {
        QRect r = screen->availableGeometry();
        if(pos.x() + width() > r.right()) {
            pos.setX(keep.left() - width());
        }
        pos.setY(qBound(r.top(), pos.y(), r.bottom() - height()));
        pos.setX(qBound(r.left(), pos.x(), r.right() - width()));
    }
    move(pos);
    placed = true;
}

void CardDialog::showMovable() {
    setWindowModality(Qt::NonModal);
    show();
    raise();
    activateWindow();
}

bool CardDialog::ask(QWidget* parent, QString question, QString detail, QString action, ButtonKind kind, QString cancel) {
    CardDialog d("", "", parent);
    d.setWindowTitle(question);
    d.body()->setContentsMargins(14, 14, 14, 4);
    auto q = d.addText(question, "card_question");
    q->setStyleSheet("font-size: 15px;");
    if(!detail.isEmpty()) {
        d.addText(detail);
    }
    auto c = d.addButton(cancel);
    auto a = d.addButton(action, kind);
    connect(c, &QPushButton::clicked, &d, &QDialog::reject);
    connect(a, &QPushButton::clicked, &d, &QDialog::accept);
    d.setMinimumWidth(340);
    d.setMaximumWidth(460);
    return d.exec() == QDialog::Accepted;
}

void CardDialog::inform(QWidget* parent, QString title, QString detail, bool ok) {
    CardDialog d("", "", parent);
    d.setWindowTitle(title);
    d.body()->setContentsMargins(14, 14, 14, 4);
    auto t = d.addText(QString(ok ? "✓ " : "✕ ") + title, "card_question");
    t->setStyleSheet(QString("font-size: 15px; color: %1;").arg(ok ? "#1a7f37" : "#b42318"));
    if(!detail.isEmpty()) {
        d.addText(detail);
    }
    auto b = d.addButton("OK", CardDialog::PRIMARY);
    connect(b, &QPushButton::clicked, &d, &QDialog::accept);
    d.setMinimumWidth(320);
    d.setMaximumWidth(460);
    d.exec();
}
