#include "chatbubble.h"
#include "ui_chatbubble.h"
#include <QTime>
#include "AircraftManager.h"
#include <QDebug>

ChatBubble::ChatBubble(QString source, QString dest, QString msg, bool sent, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatBubble)
{
    ui->setupUi(this);

    QString source_name = source;
    QColor color = palette().color(QPalette::WindowText);

    if(AircraftManager::get()->aircraftExists(source)) {
        auto ac = AircraftManager::get()->getAircraft(source);
        source_name = ac->name();
        color = ac->getColor();
    }
    if(dest != "") {
        source_name += QString::fromUtf8(" \u2794 ") + dest;
    }

    auto date = QTime::currentTime().toString();
    ui->name_label->setText("<b>"+source_name+"</b> "+date+" :");
    ui->name_label->setStyleSheet("color:" + color.name());
    ui->message_label->setText(msg);

    if(sent) {
        ui->name_label->setAlignment(Qt::AlignLeft);
        ui->horizontalLayout->setAlignment(Qt::AlignLeft);
    } else {
        ui->name_label->setAlignment(Qt::AlignRight);
        ui->horizontalLayout->setAlignment(Qt::AlignRight);
    }

    // delay set max width to wait for the widget to be shown.
    QTimer::singleShot(20, this, [=](){
            auto width = size().width() * 0.7;
            ui->frame->setMaximumWidth(width);
    });
}

ChatBubble::~ChatBubble()
{
    delete ui;
}

bool ChatBubble::eventFilter(QObject *obj, QEvent *ev) {
    (void)obj;
    (void)ev;
    if(ev->type() == QEvent::Resize) {
        auto width = size().width() * 0.7;
        ui->frame->setMaximumWidth(width);
    }

    return false;
}
