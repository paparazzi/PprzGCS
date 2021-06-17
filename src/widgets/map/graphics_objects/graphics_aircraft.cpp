#include "graphics_aircraft.h"
#include <QPainter>
#include <QPixmap>
#include <QtXml>
#include <QFile>
#include <QtSvg/QSvgRenderer>

void SetAttrRecur(QDomElement &elem, QString ac_color);


GraphicsAircraft::GraphicsAircraft(PprzPalette palette, QString icon_path, int size, QObject *parent) :
    GraphicsObject(palette, parent),
    size(size)
{
    loadSvg(icon_path);
    changeColor(palette.getColor());
}

QRectF GraphicsAircraft::boundingRect() const {
    return QRectF(-size/2*scale_factor, -size/2*scale_factor, size*scale_factor, size*scale_factor);
}

void GraphicsAircraft::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    (void)option;
    (void)widget;
    painter->setRenderHint(QPainter::Antialiasing);
    painter->drawPixmap(QRect(-size/2*scale_factor, -size/2*scale_factor, size*scale_factor, size*scale_factor), pixmap);
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

    // create svg renderer with edited contents
    QSvgRenderer svgRenderer(svgdoc.toByteArray());

    // create pixmap target (could be a QImage)
    QPixmap pix(svgRenderer.defaultSize());
    pix.fill(Qt::transparent);
    // create painter to act over pixmap
    QPainter pixPainter(&pix);
    pixPainter.setRenderHint(QPainter::Antialiasing);
    // use renderer to render over painter which paints on pixmap
    svgRenderer.render(&pixPainter);
    pixmap = pix;
}

void SetAttrRecur(QDomElement &elem, QString ac_color)
{
    if(elem.attribute("pprz") == "ac_color") {
        elem.setAttribute("fill", ac_color);
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
