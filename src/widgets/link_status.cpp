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
    // the path, from the link statistics of the aircraft and of the nodes in between
    col = 1;
    for(auto c: {"Path", "Down loss", "Up loss", "Max gap", "Signal"}) {
        auto l = new QLabel(c, this);
        l->setObjectName("link_secondary");
        l->setAlignment((col == 1 ? Qt::AlignLeft : Qt::AlignRight) | Qt::AlignVCenter);
        grid->addWidget(l, PATH_ROW, col++);
        path_captions.append(l);
        l->hide();
    }
    path_notes = new QLabel(this);
    path_notes->setObjectName("link_secondary");
    path_notes->setWordWrap(true);
    path_notes->setTextFormat(Qt::RichText);
    path_notes->hide();
    v->addLayout(grid);
    v->addWidget(path_notes);
    v->addStretch(1);

    auto path_timer = new QTimer(this);
    connect(path_timer, &QTimer::timeout, this, &LinkStatus::updatePath);
    path_timer->start(1000);

    auto ac_status = AircraftManager::get()->getAircraft(ac_id)->getStatus();
    connect(ac_status, &AircraftStatus::telemetry_status, this, &LinkStatus::updateData);
}

LinkStatus::Row LinkStatus::makeRow(QString link_id) {
    Row r;
    r.dot = new QLabel(this);
    r.dot->setFixedSize(10, 10);
    bool no_id = link_id == "no_id" || link_id == "-1";    // (link without -id)
    r.name = new QLabel(no_id ? QString("Link") : link_id, this);
    r.name->setObjectName("link_name");
    r.name->setProperty("link_id", no_id ? QString("Link") : link_id);
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

namespace {
const char* AMBER = "#b54708";
const char* RED = "#b42318";

void setValue(QLabel* l, QString text, const char* color = nullptr, bool bold = false) {
    l->setText(text);
    QString style;
    if(color) {
        style = QString("color: %1;").arg(color);
        if(bold) {
            style += " font-weight: bold;";
        }
    }
    if(l->styleSheet() != style) {
        l->setStyleSheet(style);
    }
}

QString dash() {
    return QString::fromUtf8("\xE2\x80\x94");
}
}

void LinkStatus::updatePath() {
    auto path = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getLinkPath();
    bool shown = path->hasPath();
    auto legs = shown ? path->legs() : QList<LinkPath::Leg>();
    for(auto c: path_captions) {
        c->setVisible(shown);
    }

    // one row per leg, the labels kept (no flicker)
    while(path_rows.size() > legs.size()) {
        for(auto l: path_rows.takeLast()) {
            l->deleteLater();
        }
    }
    while(path_rows.size() < legs.size()) {
        int row = PATH_ROW + 1 + path_rows.size();
        QList<QLabel*> labels;
        for(int col = 1; col <= 5; col++) {
            auto l = new QLabel(this);
            l->setObjectName(col == 1 ? "link_secondary" : "link_value");
            l->setAlignment((col == 1 ? Qt::AlignLeft : Qt::AlignRight) | Qt::AlignVCenter);
            grid->addWidget(l, row, col);
            labels.append(l);
        }
        path_rows.append(labels);
    }

    QStringList notes;
    for(int i = 0; i < legs.size(); i++) {
        auto &leg = legs[i];
        auto &r = path_rows[i];
        r[0]->setText(leg.name);
        for(int j = 0; j < 2; j++) {
            auto loss = j == 0 ? leg.down_loss : leg.up_loss;
            if(loss) {
                const char* c = *loss >= 10 ? RED : *loss >= 1 ? AMBER : nullptr;
                setValue(r[1 + j], QString("%1 %").arg(*loss, 0, 'f', *loss < 10 ? 1 : 0), c, *loss >= 10);
            } else {
                setValue(r[1 + j], dash());
            }
        }
        if(leg.max_gap) {
            double g = *leg.max_gap;
            QString t = g >= 65535 ? QString("> 65 s") : g < 10000 ? QString("%1 ms").arg(g, 0, 'f', 0)
                                                                  : QString("%1 s").arg(g / 1000, 0, 'f', 0);
            setValue(r[3], t, g >= 3000 ? RED : g >= 1000 ? AMBER : nullptr, g >= 3000);
        } else {
            setValue(r[3], dash());
        }
        // the signal: weak in amber, very weak in red
        if(leg.dbm) {
            int d = *leg.dbm;
            setValue(r[4], QString("%1 dBm").arg(d), d < -90 ? RED : d < -80 ? AMBER : nullptr, d < -80);
        } else {
            setValue(r[4], dash());
        }
        QStringList errs;
        if(leg.rejected > 0) {
            errs.append(QString("%1 rejected").arg(leg.rejected));
        }
        if(leg.unsent > 0) {
            errs.append(QString("%1 not sent").arg(leg.unsent));
        }
        if(!errs.isEmpty()) {
            notes.append(QString("%1: %2").arg(leg.name.toHtmlEscaped(), errs.join(", ")));
        }
    }

    // the nodes in between: battery (low: amber, critical: red), temperature, the GCS computers they found
    for(auto &n: shown ? path->nodes() : QList<LinkPath::Node>()) {
        if(n.hop == 0) {
            continue;
        }
        QStringList parts;
        if(n.battery) {
            int b = *n.battery;
            QString t = QString("battery %1 %").arg(b);
            if(b < 20) {
                t = QString("<span style=\"color: %1; font-weight: bold;\">%2</span>").arg(b < 10 ? RED : AMBER, t);
            }
            parts.append(t);
        }
        if(n.temperature) {
            parts.append(QString::fromUtf8("%1 \xC2\xB0""C").arg(*n.temperature));
        }
        if(n.peers > 1) {
            parts.append(QString("%1 GCS").arg(n.peers));
        }
        if(n.dropped > 0) {
            parts.append(QString("%1 dropped").arg(n.dropped));
        }
        if(!parts.isEmpty()) {
            notes.append(QString("Node %1: %2").arg(n.hop).arg(parts.join(", ")));
        }
    }
    auto missing = path->missing();
    if(!missing.isEmpty()) {
        notes.append(missing);
    }
    QString text = notes.join("<br>");
    if(path_notes->text() != text) {
        path_notes->setText(text);
    }
    path_notes->setVisible(!text.isEmpty());
}
