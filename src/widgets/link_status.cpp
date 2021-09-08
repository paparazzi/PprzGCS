#include "link_status.h"
#include "AircraftManager.h"

LinkStatus::LinkStatus(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id)
{
    grid_layout = new QGridLayout(this);

    t_link_id = new QLabel("Link id:", this);
    t_status = new QLabel("Status:", this);
    t_ping_time = new QLabel("Ping time[ms]", this);
    t_link_rx = new QLabel("Link Rx [Bytes/s]", this);
    t_downlink = new QLabel("Downlink [Byte/s]", this);
    t_uplink_lost = new QLabel("Uplink lost [s]", this);

    grid_layout->addWidget(t_link_id,     0, 0);
    grid_layout->addWidget(t_status,      1, 0);
    grid_layout->addWidget(t_ping_time,   2, 0);
    grid_layout->addWidget(t_link_rx,     3, 0);
    grid_layout->addWidget(t_downlink,    4, 0);
    grid_layout->addWidget(t_uplink_lost, 5, 0);

    auto ac_status = AircraftManager::get()->getAircraft(ac_id)->getStatus();
    connect(ac_status, &AircraftStatus::telemetry_status, this, &LinkStatus::updateData);
}

void LinkStatus::updateData() {
    auto msgs = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getTelemetryMessages();

    if(!msgs.contains("no_id") && links.contains("no_id")) {
        auto lni = links["no_id"];
        lni.link_id->deleteLater();
        lni.status->deleteLater();
        lni.ping_time->deleteLater();
        lni.link_rx->deleteLater();
        lni.downlink->deleteLater();
        lni.uplink_lost->deleteLater();
        links.remove("no_id");
    }


    for(auto it=msgs.begin(); it!=msgs.end(); ++it) {
        if(!links.contains(it.key())) {
            Status l = {
                new QLabel(it.key(), this),
                new ColorLabel(0, this),
                new QLabel(this),
                new QLabel(this),
                new QLabel(this),
                new QLabel(this)
            };
            links[it.key()] = l;

            l.link_id->setStyleSheet("font-weight: bold;");

            int col = grid_layout->columnCount();
            grid_layout->addWidget(l.link_id, 0, col, Qt::AlignCenter);
            grid_layout->addWidget(l.status, 1, col);
            grid_layout->addWidget(l.ping_time, 2, col);
            grid_layout->addWidget(l.link_rx, 3, col);
            grid_layout->addWidget(l.downlink, 4, col);
            grid_layout->addWidget(l.uplink_lost, 5, col);
        }

        auto l = links[it.key()];

        float time_since_last_msg, ping_time, rx_bytes_rate;
        uint16_t downlink_rate;
        uint32_t uplink_lost_time;
        it.value().getField("time_since_last_msg", time_since_last_msg);
        it.value().getField("ping_time", ping_time);
        it.value().getField("rx_bytes_rate", rx_bytes_rate);
        it.value().getField("downlink_rate", downlink_rate);
        it.value().getField("uplink_lost_time", uplink_lost_time);

        QString status_txt = "";
        if(time_since_last_msg > 2) {
            status_txt = QString::number(static_cast<int>(time_since_last_msg));
        }
        l.status->setText(status_txt);
        if(time_since_last_msg < 5) {
            l.status->setBrush(Qt::green);
        } else {
            l.status->setBrush(Qt::red);
        }

        l.link_rx->setText(QString::number(rx_bytes_rate));
        l.ping_time->setText(QString::number(ping_time));
        l.downlink->setText(QString::number(downlink_rate));
        l.uplink_lost->setText(QString::number(uplink_lost_time));
    }
}
