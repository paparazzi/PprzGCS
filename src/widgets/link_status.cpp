#include "link_status.h"
#include "AircraftManager.h"

LinkStatus::LinkStatus(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id)
{
    auto v = new QVBoxLayout(this);
    v->setContentsMargins(2, 0, 2, 0);
    v->setSpacing(4);
    empty_label = new QLabel("No telemetry yet", this);
    empty_label->setObjectName("link_secondary");
    v->addWidget(empty_label);

    // the captions once, above the links (not in a box per link)
    grid = new QGridLayout();
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(3);
    int col = 2;
    for(auto c: {"Ping", "Received", "Downlink", "Uplink lost"}) {
        auto l = new QLabel(c, this);
        l->setObjectName("link_secondary");
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        grid->addWidget(l, 0, col++);
        captions.append(l);
        l->hide();
    }
    v->addLayout(grid);
    v->addStretch(1);

    auto ac_status = AircraftManager::get()->getAircraft(ac_id)->getStatus();
    connect(ac_status, &AircraftStatus::telemetry_status, this, &LinkStatus::updateData);
}

LinkStatus::Row LinkStatus::makeRow(QString link_id) {
    Row r;
    r.dot = new QLabel(this);
    r.dot->setFixedSize(10, 10);
    r.name = new QLabel(link_id == "no_id" ? QString("Link") : link_id, this);
    r.name->setObjectName("link_name");
    r.name->setProperty("link_id", link_id == "no_id" ? QString("Link") : link_id);
    auto value = [&]() {
        auto l = new QLabel(QString::fromUtf8("\xE2\x80\x94"), this);
        l->setObjectName("link_value");
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        return l;
    };
    r.ping = value();
    r.rx = value();
    r.downlink = value();
    r.uplink_lost = value();
    int row = next_row++;
    grid->addWidget(r.dot, row, 0, Qt::AlignVCenter);
    grid->addWidget(r.name, row, 1);
    grid->addWidget(r.ping, row, 2);
    grid->addWidget(r.rx, row, 3);
    grid->addWidget(r.downlink, row, 4);
    grid->addWidget(r.uplink_lost, row, 5);
    return r;
}

void LinkStatus::updateData() {
    auto msgs = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getTelemetryMessages();
    empty_label->setVisible(msgs.isEmpty());
    for(auto c: captions) {
        c->setVisible(!msgs.isEmpty());
    }

    // links that are gone
    for(auto &id: rows.keys()) {
        if(!msgs.contains(id)) {
            for(auto w: rows[id].widgets()) {
                w->deleteLater();
            }
            rows.remove(id);
        }
    }

    for(auto it=msgs.begin(); it!=msgs.end(); ++it) {
        if(!rows.contains(it.key())) {
            rows[it.key()] = makeRow(it.key());
        }
        auto &r = rows[it.key()];

        float time_since_last_msg = 0, ping_time = 0, rx_bytes_rate = 0;
        uint16_t downlink_rate = 0;
        uint32_t uplink_lost_time = 0;
        it.value().getField("time_since_last_msg", time_since_last_msg);
        it.value().getField("ping_time", ping_time);
        it.value().getField("rx_bytes_rate", rx_bytes_rate);
        it.value().getField("downlink_rate", downlink_rate);
        it.value().getField("uplink_lost_time", uplink_lost_time);

        // OK: the green dot; lost: red, and for how long by the name
        bool ok = time_since_last_msg < 5;
        QString color = ok ? "#1f7a3f" : "#b42318";
        r.dot->setStyleSheet(QString("background: %1; border-radius: 5px;").arg(color));
        QString name = r.name->property("link_id").toString();
        r.name->setText(ok ? name : QString("%1 · lost %2 s").arg(name).arg(static_cast<int>(time_since_last_msg)));
        r.name->setStyleSheet(ok ? "" : QString("color: %1;").arg(color));
        r.ping->setText(QString("%1 ms").arg(ping_time, 0, 'f', 0));
        r.rx->setText(QString("%1 B/s").arg(rx_bytes_rate, 0, 'f', 0));
        r.downlink->setText(QString("%1 B/s").arg(downlink_rate));
        r.uplink_lost->setText(QString("%1 s").arg(uplink_lost_time));
        r.uplink_lost->setStyleSheet(uplink_lost_time > 5 ? "color: #b42318; font-weight: bold;" : "");
    }
}
