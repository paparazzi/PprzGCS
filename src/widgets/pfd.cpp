#include "pfd.h"
#include <QPaintEvent>
#include <QPainter>
#include <math.h>
#include <QDebug>
#include <QApplication>
#include <algorithm>
#include <AircraftManager.h>
#include "dispatcher_ui.h"
#include <QPixmap>
#include <QBitmap>
#include <QRegion>


Pfd::Pfd(QWidget *parent) : QWidget(parent), border_stroke(6)
{
    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this, &Pfd::changeCurrentAC);
    connect(PprzDispatcher::get(), &PprzDispatcher::flight_param, this, &Pfd::updateEulers);
    places[0] = ROLL;
    places[1] = PITCH;
    places[2] = YAW;
}

void Pfd::resizeEvent(QResizeEvent *event) {
    (void)event;
    QRegion reg;
    for(int i=0; i<3; i++){
        QRect rect = placeRect(i);
        rect.setWidth(rect.width()+border_stroke);
        rect.setHeight(rect.height()+border_stroke);
        QPointF center = placeCenter(i);
        QPoint p(static_cast<int>(center.x()), static_cast<int>(center.y()));
        rect.moveCenter(p);

        QRegion ri(rect, QRegion::Ellipse);
        reg = reg.united(ri);
    }

    setMask(reg);
}


void Pfd::paintEvent(QPaintEvent *event) {
    (void)event;

    int side = qMin(width(), height());
    QRect rect = QRect(-side/4,-side/4, side/2, side/2);

    if(eulers.find(current_ac) != eulers.end()) {

        for(int i=0; i<3; i++) {
            QRect rect = placeRect(i);
            QPointF center = placeCenter(i);
            if(places[i] == PITCH) {
                paintPitch(rect, center);
            } else if(places[i] == ROLL) {
                paintRoll(rect, center);
            } else {
                paintYaw(rect, center);
            }
        }

    } else {
        QPainter painter(this);
        painter.setBrush(QBrush(QColor("#8ed3ea")));
        painter.drawEllipse(rect);
        painter.setBrush(QColor("#98845b"));
        painter.drawChord(rect, static_cast<int>(16*(M_PI)*180.0/M_PI), static_cast<int>(16*(M_PI)*180.0/M_PI));
        QPen pen = QPen(Qt::red, 4);
        painter.setPen(pen);
        QLine line(rect.left(), rect.center().y(), rect.right(), rect.center().y());
        painter.drawLine(line);
    }



}

void Pfd::paintPitch(QRect rect,  QPointF center) {
    float pitch = eulers[current_ac].pitch;
    float roll = eulers[current_ac].roll;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(center);

    painter.setBrush(QBrush(QColor("#00b0ea")));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(rect);

    double alpha = -roll * M_PI/180.0;
    double y = sin(alpha) * rect.height() / 2;
    double x = cos(alpha) * rect.width() / 2 - border_stroke;

    painter.setBrush(QColor("#986701"));
    painter.drawChord(rect, 16*(alpha - M_PI)*180.0/M_PI, 16*(2*(M_PI_2-alpha))*180.0/M_PI);
    //painter.drawChord(rect, 16*(-M_PI)*180.0/M_PI, 16*(2*(M_PI_2))*180.0/M_PI);

    QPen pen = QPen(Qt::white, 4);
    painter.setPen(pen);
    QLine line(-x, y , x , y);
    //QLine line(rect.left(), 0 , rect.right() , 0);
    painter.drawLine(line);

    pen = QPen(Qt::black, 2, Qt::DashDotLine);
    painter.setPen(pen);
    line = QLine(rect.left()+border_stroke, 0 , rect.right()-border_stroke , 0);
    painter.drawLine(line);

    QPixmap pix(":/pictures/pfd_aircraft_pitch.svg");

    //painter.translate(rect.center());
    painter.rotate(pitch);
    painter.drawPixmap(-rect.width()/2, -rect.height()/2, rect.width(), rect.height(), pix.scaled(pix.width(),pix.height(),Qt::KeepAspectRatio));

    QPainterPath pp;
    pp.addEllipse(rect);
    pp.addEllipse(rect.center(), rect.left()+border_stroke, rect.top()+border_stroke);
    painter.setBrush(AircraftManager::get()->getAircraft(current_ac).getColor());
    painter.setPen(Qt::NoPen);
    painter.drawPath(pp);
}

void Pfd::paintRoll(QRect rect,  QPointF center) {
    float pitch = eulers[current_ac].pitch;
    float roll = eulers[current_ac].roll;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(center);
    painter.setBrush(QBrush(QColor("#00b0ea")));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(rect);

    double alpha = pitch * M_PI/180.0;
    double y = sin(alpha) * rect.height() / 2;
    double x = cos(alpha) * rect.width() / 2 - border_stroke;

    painter.setBrush(QColor("#986701"));
    painter.drawChord(rect, 16*(alpha - M_PI)*180.0/M_PI, 16*(2*(M_PI_2-alpha))*180.0/M_PI);

    QPen pen = QPen(Qt::white, 4);
    painter.setPen(pen);
    QLine line(-x, y , x , y);
    painter.drawLine(line);

    pen = QPen(Qt::black, 2, Qt::DashDotLine);
    painter.setPen(pen);
    line = QLine(rect.left()+border_stroke, 0 , rect.right()-border_stroke , 0);
    painter.drawLine(line);

    QPixmap pix(":/pictures/pfd_aircraft_roll.svg");

    //painter.translate(rect.center());
    painter.rotate(roll);
    painter.drawPixmap(-rect.width()/2, -rect.height()/2, rect.width(), rect.height(), pix.scaled(pix.width(),pix.height(),Qt::KeepAspectRatio));

    QPainterPath pp;
    pp.addEllipse(rect);
    pp.addEllipse(rect.center(), rect.left()+border_stroke, rect.top()+border_stroke);
    painter.setBrush(AircraftManager::get()->getAircraft(current_ac).getColor());
    painter.setPen(Qt::NoPen);
    painter.drawPath(pp);
}

void Pfd::paintYaw(QRect rect, QPointF center) {
    float yaw = eulers[current_ac].yaw;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(center);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor("#986701")));
    painter.drawEllipse(rect);

    QPen pen = QPen(Qt::black, 2, Qt::DashDotLine);
    painter.setPen(pen);
    QLine line = QLine(rect.left()+border_stroke, 0 , rect.right()-border_stroke , 0);
    painter.drawLine(line);

    QPixmap pix(":/pictures/pfd_aircraft_yaw.svg");

    //painter.translate(rect.center());
    painter.rotate(yaw);
    painter.drawPixmap(-rect.width()/2, -rect.height()/2, rect.width(), rect.height(), pix.scaled(pix.width(),pix.height(),Qt::KeepAspectRatio));

    QPainterPath pp;
    pp.addEllipse(rect);
    pp.addEllipse(rect.center(), rect.left()+border_stroke, rect.top()+border_stroke);
    painter.setBrush(AircraftManager::get()->getAircraft(current_ac).getColor());
    painter.setPen(Qt::NoPen);
    painter.drawPath(pp);
}


void Pfd::mousePressEvent(QMouseEvent *event) {
    (void)event;
    for(int i=2; i>=0; i--){
        QRect rect = placeRect(i);
        QPointF center = placeCenter(i);
        QPoint p(static_cast<int>(center.x()), static_cast<int>(center.y()));
        rect.moveCenter(p);
        if(rect.contains(event->pos())) {
            double r = sqrt(pow(event->pos().x()-center.x(), 2) + pow(event->pos().y()-center.y(), 2));
            if(r < rect.width()/2) {
                Axis tmp = places[0];
                places[0] = places[i];
                places[i] = tmp;
                update();
                return;
            }
        }
    }

    event->ignore();
    QWidget::mousePressEvent(event);
}

//void Pfd::mouseMoveEvent(QMouseEvent *event) {
//    event->ignore();
//    QWidget::mouseMoveEvent(event);
//}

//void Pfd::mouseReleaseEvent(QMouseEvent *event) {
//    event->ignore();

//    QWidget::mouseReleaseEvent(event);
//}

void Pfd::changeCurrentAC(QString id) {
    if(AircraftManager::get()->aircraftExists(id)) {
        current_ac = id;
        update();
    }
}

void Pfd::updateEulers(pprzlink::Message msg) {
    std::string ac_id;
    float roll, pitch, course;
    msg.getField("ac_id", ac_id);
    if(AircraftManager::get()->aircraftExists(ac_id.c_str())) {
        msg.getField("roll", roll);
        msg.getField("pitch", pitch);
        msg.getField("course", course);
        eulers[ac_id.c_str()] = Eulers {roll, pitch, course};
        update();
    }
}


QRect Pfd::placeRect(int i) {
    int side = qMin(width(), height());

    if(i==0) {
        double sizeA = side*0.8;
        return QRect(-sizeA/2,-sizeA/2, sizeA, sizeA);
    } else if(i==1) {
        double sizeB = side*0.35;
        return QRect(-sizeB/2,-sizeB/2, sizeB, sizeB);
    } else {
        double sizeC = side*0.35;
        return QRect(-sizeC/2,-sizeC/2, sizeC, sizeC);
    }

    throw runtime_error("PFD place do not exist!");
}

QPointF Pfd::placeCenter(int i) {
    int side = qMin(width(), height());
    double dx = (width()-side)/2;
    double dy = (height()-side)/2;
    QRectF pfd_rect = QRectF(dx,dy, side, side);

    if(i==0) {
        double sizeA = side*0.8;
        return QPointF(pfd_rect.left() + sizeA/2, pfd_rect.center().y());
    } else if(i==1) {
        double sizeB = side*0.35;
        QRect rectB = QRect(-sizeB/2,-sizeB/2, sizeB, sizeB);
        return pfd_rect.topRight()-rectB.topRight();
    } else {
        double sizeC = side*0.35;
        QRect rectC = QRect(-sizeC/2,-sizeC/2, sizeC, sizeC);
        return pfd_rect.bottomRight() - rectC.bottomRight();
    }

    throw runtime_error("PFD place do not exist!");
}

QSize Pfd::sizeHint() const
{
    return QSize(200, 200);
}

QSize Pfd::minimumSizeHint() const
{
    return QSize(200, 200);
}
