#include "link_status.h"
#include "AircraftManager.h"

LinkStatus::LinkStatus(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id)
{
    rows_layout = new QVBoxLayout(this);
    rows_layout->setContentsMargins(0, 0, 0, 0);
    rows_layout->setSpacing(8);
    empty_label = new QLabel("No telemetry yet", this);
    empty_label->setObjectName("link_secondary");
    rows_layout->addWidget(empty_label);
    setMinimumWidth(240);

    auto ac_status = AircraftManager::get()->getAircraft(ac_id)->getStatus();
    connect(ac_status, &AircraftStatus::telemetry_status, this, &LinkStatus::updateData);
}

LinkStatus::Row LinkStatus::makeRow(QString link_id) {
    Row r;
    r.widget = new QWidget(this);
    r.widget->setObjectName("link_row");
    r.widget->setAttribute(Qt::WA_StyledBackground);
    auto v = new QVBoxLayout(r.widget);
    v->setContentsMargins(8, 6, 8, 6);
    v->setSpacing(4);

    auto head = new QHBoxLayout();
    r.dot = new QLabel(r.widget);
    r.dot->setFixedSize(12, 12);
    r.name = new QLabel(link_id == "no_id" ? QString("Link") : link_id, r.widget);
    r.name->setObjectName("link_name");
    r.state = new QLabel(r.widget);
    r.state->setObjectName("link_state");
    head->addWidget(r.dot);
    head->addWidget(r.name, 1);
    head->addWidget(r.state);
    v->addLayout(head);

    auto grid = new QGridLayout();
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(0);
    auto cell = [&](int col, QString caption, QLabel*& value) {
        auto c = new QLabel(caption, r.widget);
        c->setObjectName("link_secondary");
        value = new QLabel(QString::fromUtf8("\xE2\x80\x94"), r.widget);
        value->setObjectName("link_value");
        grid->addWidget(c, 0, col);
        grid->addWidget(value, 1, col);
    };
    cell(0, "Ping", r.ping);
    cell(1, "Received", r.rx);
    cell(2, "Downlink", r.downlink);
    cell(3, "Uplink lost", r.uplink_lost);
    v->addLayout(grid);
    rows_layout->addWidget(r.widget);
    return r;
}

void LinkStatus::updateData() {
    auto msgs = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getTelemetryMessages();
    empty_label->setVisible(msgs.isEmpty());

    // links that are gone
    for(auto &id: rows.keys()) {
        if(!msgs.contains(id)) {
            rows[id].widget->deleteLater();
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

        bool ok = time_since_last_msg < 5;
        QString color = ok ? "#1f7a3f" : "#b42318";
        r.dot->setStyleSheet(QString("background: %1; border-radius: 6px;").arg(color));
        r.state->setText(ok ? "OK" : QString("Lost %1 s").arg(static_cast<int>(time_since_last_msg)));
        r.state->setStyleSheet(QString("color: %1; font-weight: bold;").arg(color));
        r.ping->setText(QString("%1 ms").arg(ping_time, 0, 'f', 0));
        r.rx->setText(QString("%1 B/s").arg(rx_bytes_rate, 0, 'f', 0));
        r.downlink->setText(QString("%1 B/s").arg(downlink_rate));
        r.uplink_lost->setText(QString("%1 s").arg(uplink_lost_time));
        QString uplink_color = uplink_lost_time > 5 ? "#b42318" : "";
        r.uplink_lost->setStyleSheet(uplink_color.isEmpty() ? "" : "color: " + uplink_color + "; font-weight: bold;");
    }
}
