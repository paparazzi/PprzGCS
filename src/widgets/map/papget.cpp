#include "papget.h"
#include <QPainter>
#include <QDebug>
#include <variant>
#include <QApplication>
#include <mapwidget.h>
#include <type_traits>

//TODO must unbind message in destructor, somehow

Papget::Papget(struct DataDef datadef, QPoint pos_view, QObject *parent) : QObject(parent), QGraphicsItem(),
    datadef(datadef), type(Type::NONE), style(Style::TEXT), pos_view(pos_view)
{
    bindRet = PprzDispatcher::get()->bind(datadef.msg_name.toStdString(),
        [=](std::string sender, pprzlink::Message msg) {
            QTimer* timer = new QTimer();
            timer->moveToThread(qApp->thread());
            timer->setSingleShot(true);
            QObject::connect(timer, &QTimer::timeout, [=]()
            {
                // main thread
                callback(sender, msg);
                timer->deleteLater();
            });
            QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));

        });
}


void Papget::callback(std::string sender, pprzlink::Message msg) {
    QString msg_name = msg.getDefinition().getName().c_str();
    if(QString(sender.c_str()) == datadef.ac_id && msg_name == datadef.msg_name) {
        //qDebug() << sender.c_str();
        pprzlink::MessageDefinition msg_def = PprzDispatcher::get()->getDict()->getDefinition(datadef.msg_name.toStdString());
        pprzlink::MessageField field = msg_def.getField(datadef.field.toStdString());

// types can be:
//        CHAR,
//        INT8,
//        INT16,
//        INT32,
//        UINT8,
//        UINT16,
//        UINT32,
//        FLOAT,
//        STRING
//  and some array of that types (I think)


        //field.getType().getBaseType().
        //int plop;
        //pprzlink::FieldValue fv;
        //std::any plop;
        //std::variant<uint8_t, uint16_t, std::string> plop;
        // TODO: get field value, and type ?
        //msg.getField(datadef.field.toStdString(), plop);



        if( try_this<uint8_t, uint32_t>(msg, val_u32)  ||
            try_this<uint16_t, uint32_t>(msg, val_u32) ||
            try_this<uint32_t, uint32_t>(msg, val_u32)) {

            type = Type::U32;
        }
        else if( try_this<int8_t, int32_t>(msg, val_32)   ||
            try_this<int16_t, int32_t>(msg, val_32)  ||
            try_this<int32_t, int32_t>(msg, val_32)) {

            type = Type::I32;
        }
        else if( try_this<float, double>(msg, val_float)) {
            type = Type::FLOAT;
        }
        else if( try_this<std::string, std::string>(msg, val_str)) {
            type = Type::STR;
        }
        else if( try_this<char, char>(msg, val_char)) {
            type = Type::CHAR;
        } else {
            qDebug() << "type not handled !";
        }

        prepareGeometryChange();

    }
}

template<typename T, typename U>
bool Papget::try_this(pprzlink::Message msg, U &dst) {
    static_assert(std::is_convertible<T, U>());
    try{
        T value;
        msg.getField(datadef.field.toStdString(), value);
        dst = value;
        return true;
    } catch (std::bad_any_cast &ex) {
        return false;
    }
}


void Papget::updateGraphics(MapWidget* map) {
    scale_factor = 1.0/map->scaleFactor();
    QPointF pos_scene = map->mapToScene(pos_view);
    setPos(pos_scene);
}

QRectF Papget::boundingRect() const {
    return  bounding_rect.marginsAdded(QMarginsF(20, 20, 20, 20));
    //return QRectF(0, 0, 100, 50);
}

void Papget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    if(style == Style::TEXT) {
        paint_text(painter, option, widget);
    }
}

void Papget::paint_text(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    (void)option;
    (void)widget;

    QString text;
    if(type == U32) {
        text = QString::number(val_u32);
    } else if(type == I32) {
        text = QString::number(val_32);
    } else if(type == FLOAT) {
        text = QString::number(val_float);
    } else if(type == STR) {
        text = QString(val_str.c_str());
    } else if(type == CHAR) {
        text = QString(val_char);
    } else if (type == NONE) {
        qDebug() << "Papget: no good value";
    }

    QPainterPath path_draw;
    QFont font;
    font.setPointSize (static_cast<int>(qApp->property("MAPITEMS_FONT").toInt()*scale_factor));
    font.setWeight(QFont::DemiBold);
    path_draw.addText(0, 0, font, text);
    painter->setBrush(Qt::red);
    painter->setPen(Qt::NoPen);
    painter->drawPath(path_draw);

    QFontMetrics fm(font);

    double w = fm.horizontalAdvance(text);
    auto h = fm.height();

    bounding_rect = QRectF(0, 0, w*scale_factor, h*scale_factor);
}

//QPainterPath Papget::shape() const {
//        return QGraphicsItem::shape();
//}
