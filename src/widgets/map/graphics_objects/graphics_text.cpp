#include "graphics_text.h"
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QTextDocument>

GraphicsText::GraphicsText(const QString &text, PprzPalette palette,QObject *parent):
    GraphicsObject(palette, parent),
    QGraphicsTextItem (text)

{
    setDefaultTextColor(palette.getColor());
    // readable on any background: a dark glow around light text, a white one around dark text
    auto glow = new QGraphicsDropShadowEffect();
    glow->setOffset(0, 0);
    glow->setBlurRadius(5);
    bool light = palette.getColor().lightness() > 150;
    glow->setColor(light ? QColor(0, 0, 0, 230) : QColor(255, 255, 255, 240));
    setGraphicsEffect(glow);
}

void GraphicsText::changeFocus() {
    switch (style) {
    case DEFAULT:
        if(isHighlighted()) {
            setVisible(true);
        } else {
            setVisible(false);
        }

        break;
    case CARROT:
        setVisible(false);
        break;
    case CURRENT_NAV:
        setVisible(false);
        break;
    case GCS:
        setVisible(false);
        break;
    case DCSHOT:
        setVisible(false);
        break;
    case CRASH:
        setVisible(false);
        break;
    }

    update();
}

void GraphicsText::setStyle(Style s) {
    GraphicsObject::setStyle(s);
    changeFocus();
}


void GraphicsText::setPill(bool p) {
    if(p == pill) {
        return;
    }
    pill = p;
    if(graphicsEffect()) {
        graphicsEffect()->setEnabled(!pill);
    }
    // the text is drawn bold in the pill: measured with that font, so it fits
    prepareGeometryChange();
    auto f = font();
    f.setWeight(pill ? QFont::DemiBold : QFont::Normal);
    setFont(f);
    update();
}

void GraphicsText::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    if(!pill) {
        QGraphicsTextItem::paint(painter, option, widget);
        return;
    }
    // a rounded pill in the color, a white border and white text
    QRectF r = boundingRect().adjusted(1, 2, -1, -2);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 50));
    painter->drawRoundedRect(r.translated(0, 1.2), r.height() / 2, r.height() / 2);
    // a bit see-through, the map still shows
    QColor fill = defaultTextColor();
    fill.setAlpha(200);
    painter->setPen(QPen(QColor(255, 255, 255, 220), 1.5));
    painter->setBrush(fill);
    painter->drawRoundedRect(r, r.height() / 2, r.height() / 2);
    QFont f = font();
    QFontMetricsF fm(f);
    QPainterPath path;
    double m = document()->documentMargin();
    path.addText(m, r.center().y() + (fm.ascent() - fm.descent()) / 2, f, toPlainText());
    painter->fillPath(path, Qt::white);
}
