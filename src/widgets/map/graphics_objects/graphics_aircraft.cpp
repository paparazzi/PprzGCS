#include "graphics_aircraft.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <cmath>
#include <QPixmap>
#include <QtXml>
#include <QFile>
#include <QtSvg/QSvgRenderer>
#include <QRegularExpression>

void SetAttrRecur(QDomElement &elem, QString ac_color);


GraphicsAircraft::GraphicsAircraft(PprzPalette palette, QString icon_path, int size, QObject *parent) :
    GraphicsObject(palette, parent),
    size(size)
{
    loadSvg(icon_path);
    changeColor(palette.getColor());
    setAcceptHoverEvents(true);     // its name shows while hovering
}

void GraphicsAircraft::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    QGraphicsItem::hoverEnterEvent(event);
    emit hoverChanged(true);
}

void GraphicsAircraft::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    // a single press goes on to the map (panning); a double click selects
    event->ignore();
}

void GraphicsAircraft::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    emit objectDoubleClicked(event->scenePos());
    event->accept();
}

bool GraphicsAircraft::sceneEventFilter(QGraphicsItem* watched, QEvent* event) {
    (void)watched;
    if(event->type() == QEvent::GraphicsSceneMouseDoubleClick) {
        mouseDoubleClickEvent(static_cast<QGraphicsSceneMouseEvent*>(event));
        return true;
    }
    return false;
}

void GraphicsAircraft::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    QGraphicsItem::hoverLeaveEvent(event);
    emit hoverChanged(false);
}

// the icon is drawn with a margin for its outline and shadow (in fractions of its size)
static constexpr double HALO_MARGIN = 0.12;

QRectF GraphicsAircraft::boundingRect() const {
    double s = size * scale_factor * (1 + 2 * HALO_MARGIN);
    return QRectF(-s/2, -s/2, s, s);
}

void GraphicsAircraft::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    (void)option;
    (void)widget;
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->drawPixmap(boundingRect(), pixmap, QRectF(pixmap.rect()));
}

void GraphicsAircraft::changeFocus() {
    update();
}

void GraphicsAircraft::loadSvg(QString path) {
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QByteArray baData = file.readAll();
    // load svg contents to xml document and edit contents
    svgdoc.setContent(baData);
}

void GraphicsAircraft::changeColor(QColor color) {
    auto el = svgdoc.documentElement();
    // recusivelly change color
    SetAttrRecur(el, color.name());

    // rendered big (sharp when scaled down), in a square keeping the icon's aspect
    QSvgRenderer svgRenderer(svgdoc.toByteArray());
    const int res = 256;
    QImage icon(res, res, QImage::Format_ARGB32_Premultiplied);
    icon.fill(Qt::transparent);
    {
        QPainter ip(&icon);
        ip.setRenderHint(QPainter::Antialiasing);
        QSizeF ds = svgRenderer.defaultSize();
        double k = (ds.width() > 0 && ds.height() > 0) ? res / std::max(ds.width(), ds.height()) : 1;
        QSizeF s = ds * k;
        svgRenderer.render(&ip, QRectF((res - s.width()) / 2, (res - s.height()) / 2, s.width(), s.height()));
    }
    // its shape in one color
    auto tinted = [&](QColor c) {
        QImage t = icon;
        QPainter tp(&t);
        tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
        tp.fillRect(t.rect(), c);
        return t;
    };
    QImage white = tinted(Qt::white);
    QImage black = tinted(QColor(0, 0, 0));

    // a soft shadow, a white outline, then the icon: visible on any background
    int margin = static_cast<int>(res * HALO_MARGIN);
    QImage out(res + 2 * margin, res + 2 * margin, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    QPainter op(&out);
    op.setRenderHint(QPainter::SmoothPixmapTransform);
    const double r = res * 0.028;   // outline width
    op.setOpacity(0.08);
    for(int a=0; a<360; a+=30) {
        double rad = a * M_PI / 180;
        op.drawImage(QPointF(margin + (r + 3) * cos(rad), margin + (r + 3) * sin(rad) + res * 0.02), black);
    }
    // a crisp outline: the shape in white, moved around it on two rings
    op.setOpacity(1);
    for(double rr: {r * 0.5, r}) {
        for(int a=0; a<360; a+=12) {
            double rad = a * M_PI / 180;
            op.drawImage(QPointF(margin + rr * cos(rad), margin + rr * sin(rad)), white);
        }
    }
    op.drawImage(QPoint(margin, margin), icon);
    op.end();
    pixmap = QPixmap::fromImage(out);
}

void SetAttrRecur(QDomElement &elem, QString ac_color)
{
    if(elem.attribute("pprz") == "ac_color") {
        elem.setAttribute("fill", ac_color);
        // a fill in the style attribute wins over the fill attribute: change it there too
        // (e.g. the rotorcraft icon, which showed red for every aircraft)
        if(elem.hasAttribute("style")) {
            auto style = elem.attribute("style");
            style.replace(QRegularExpression("(^|;)\\s*fill\\s*:[^;]*"), "\\1fill:" + ac_color);
            elem.setAttribute("style", style);
        }
    }
    // loop all children
    for (int i = 0; i < elem.childNodes().count(); i++)
    {
        if (elem.childNodes().at(i).isElement())
        {
            auto el = elem.childNodes().at(i).toElement();
            SetAttrRecur(el, ac_color);
        }
    }
}
