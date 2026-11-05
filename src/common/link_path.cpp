#include "link_path.h"
#include "pprz_dispatcher.h"
#include <cmath>
#include <tuple>

namespace {

constexpr double WINDOW = 10.0;         // s: the rates and the loss over this time
constexpr double MIN_FRAMES = 20;       // loss from counters reported at different times: from this many frames
constexpr double NODE_TIMEOUT = 60.0;   // s: a node not heard for longer leaves the path
constexpr int UNKNOWN = 127;
constexpr int NO_BATTERY = 255;

const QStringList DATALINK_REPORT = {"uplink_lost_time", "uplink_nb_msgs", "downlink_nb_msgs", "downlink_rate",
                                     "uplink_rate", "downlink_ovrn", "uplink_err", "uplink_ovrn"};
const QStringList LINK_STATS = {"hop", "uptime", "down_rx", "down_rx_err", "down_tx", "down_tx_err", "up_rx",
                                "up_rx_err", "up_tx", "up_tx_err", "down_max_gap", "up_max_gap", "nb_peers", "dropped",
                                "rssi", "down_rx_bytes", "up_rx_bytes", "rssi_ac", "battery", "temperature"};
const QStringList LINK_REPORT = {"run_time", "rx_lost_time", "rx_bytes", "rx_msgs", "rx_err", "rx_bytes_rate",
                                 "rx_msgs_rate", "tx_msgs", "ping_time", "tx_err", "rx_max_gap", "tx_bytes"};

const QHash<QString, int> WRAP_DATALINK = {{"uplink_nb_msgs", 16}, {"downlink_nb_msgs", 16}, {"downlink_ovrn", 8},
                                           {"uplink_err", 8}, {"uplink_ovrn", 8}};
const QHash<QString, int> WRAP_STATS = {{"down_rx", 16}, {"down_rx_err", 16}, {"down_tx", 16}, {"down_tx_err", 16},
                                        {"up_rx", 16}, {"up_rx_err", 16}, {"up_tx", 16}, {"up_tx_err", 16},
                                        {"dropped", 16}, {"down_rx_bytes", 16}, {"up_rx_bytes", 16}};

// generic counter names of a path end -> field of its report
const QHash<QString, QString> KEYS_DATALINK = {{"down_tx", "downlink_nb_msgs"}, {"down_tx_err", "downlink_ovrn"},
                                               {"up_rx", "uplink_nb_msgs"}, {"up_rx_err", "uplink_err"}};
const QHash<QString, QString> KEYS_LINK = {{"down_rx", "rx_msgs"}, {"down_rx_err", "rx_err"},
                                           {"down_max_gap", "rx_max_gap"}, {"up_tx", "tx_msgs"},
                                           {"up_tx_err", "tx_err"}};

double now() {
    return QDateTime::currentMSecsSinceEpoch() / 1000.0;
}

LinkPath::Values fields(const pprzlink::Message &msg, const QStringList &names) {
    LinkPath::Values v;
    for(auto &name: names) {
        try {
            bool ok;
            double d = msg.getFieldAsStr(name).toDouble(&ok);
            if(ok) {
                v[name] = d;
            }
        } catch(std::exception&) {
            // an older message definition, without this field
        }
    }
    return v;
}

using Opt = std::optional<double>;

/// One end of a leg: the counters of a report source under the generic names (those of LINK_STATS)
struct End {
    enum Kind {AP, NODE, LINK};
    QString name;
    const LinkPath::Source* src;
    Kind kind;
    bool fresh;

    QString key(QString k) const {
        if(!fresh) {
            return QString();
        }
        if(kind == NODE) {
            return k;
        }
        return (kind == AP ? KEYS_DATALINK : KEYS_LINK).value(k);
    }
    Opt rate(QString k) const {
        auto kk = key(k);
        if(kk.isEmpty()) {
            return std::nullopt;
        }
        auto r = src->rate(kk);
        if(r && *r == 0 && kind == AP && k == "down_tx") {
            return std::nullopt;    // firmware that does not count its frames (pprzlink before the link statistics)
        }
        return r;
    }
    Opt delta(QString k) const {
        auto kk = key(k);
        return kk.isEmpty() ? std::nullopt : src->delta(kk);
    }
    Opt max(QString k) const {
        auto kk = key(k);
        return kk.isEmpty() ? std::nullopt : src->max(kk);
    }
    Opt last(QString k) const {
        auto kk = key(k);
        return kk.isEmpty() ? std::nullopt : src->last(kk);
    }
    QList<double> times(QString k) const {
        auto kk = key(k);
        return kk.isEmpty() ? QList<double>() : src->times(kk);
    }
    Opt rateBetween(QString k, double t0, double t1) const {
        auto kk = key(k);
        if(kk.isEmpty()) {
            return std::nullopt;
        }
        auto c0 = src->counterAt(kk, t0);
        auto c1 = src->counterAt(kk, t1);
        if(!c0 || !c1) {
            return std::nullopt;
        }
        return (*c1 - *c0) / (t1 - t0);
    }
    /// signal reported by a node
    std::optional<int> signal(QString k) const {
        auto v = kind == NODE ? last(k) : std::nullopt;
        if(!v || static_cast<int>(*v) == UNKNOWN) {
            return std::nullopt;
        }
        return static_cast<int>(*v);
    }
};

/// Rates of counter kx of x and ky of y over the same time span, so that reports at different times
/// (DATALINK_REPORT every 5 s, LINK_REPORT every second) do not look like loss, and the number of frames
/// sent in that span (0 if too short to compare)
std::tuple<Opt, Opt, double> pairRates(const End &x, QString kx, const End &y, QString ky) {
    auto rx = x.rate(kx);
    auto ry = y.rate(ky);
    if(!rx || !ry) {
        return {rx, ry, 0};
    }
    auto tx = x.times(kx);
    auto ty = y.times(ky);
    double lo = std::max(tx.first(), ty.first());
    double hi = std::min(tx.last(), ty.last());
    // the report times of the slower end, the counter of the faster end interpolated at those times
    QList<double> ts;
    for(auto t: tx.size() <= ty.size() ? tx : ty) {
        if(t >= lo && t <= hi) {
            ts.append(t);
        }
    }
    double t0 = ts.size() >= 2 ? ts.first() : lo;
    double t1 = ts.size() >= 2 ? ts.last() : hi;
    if(t1 - t0 < 1.0) {
        return {rx, ry, 0};
    }
    auto sent = x.rateBetween(kx, t0, t1);
    auto got = y.rateBetween(ky, t0, t1);
    double n = sent.value_or(0) * (t1 - t0);
    if(n < MIN_FRAMES) {
        return {rx, ry, n};     // too few frames (e.g. the uplink, a PING every 5 s): each end over its own reports
    }
    return {sent, got, n};
}

Opt loss(Opt sent, Opt got, double n) {
    if(!sent || !got || *sent <= 0 || n < MIN_FRAMES) {
        return std::nullopt;
    }
    return std::max(0.0, 100.0 * (1.0 - *got / *sent));
}

LinkPath::Leg leg(const End &a, const End &b, bool several_links) {
    LinkPath::Leg l;
    l.name = a.name + QString::fromUtf8(" \xE2\x80\x93 ") + b.name;

    // down: a sends, b receives (a node's own reports are sent on top of what it receives)
    Opt sent, got;
    double n;
    if(b.kind == End::LINK && a.kind == End::NODE) {
        Opt r;
        std::tie(r, got, n) = pairRates(a, "down_rx", b, "down_rx");
        if(r) {
            sent = *r + a.src->reportRate().value_or(0);
        }
    } else {
        std::tie(sent, got, n) = pairRates(a, "down_tx", b, "down_rx");
    }
    l.down_loss = loss(sent, got, n);

    // up: b sends, a receives (only this computer's uplink is known)
    std::tie(sent, got, n) = pairRates(b, "up_tx", a, "up_rx");
    if(b.kind == End::LINK && (a.last("nb_peers").value_or(0) > 1 || several_links)) {
        sent = std::nullopt;
    }
    l.up_loss = loss(sent, got, n);

    l.max_gap = b.max("down_max_gap");
    // the signal of the leg, as its receiving end reports it: towards the aircraft, or towards the ground
    l.dbm = b.signal("rssi_ac");
    if(!l.dbm) {
        l.dbm = a.signal("rssi");
    }
    l.rejected = static_cast<int>(b.delta("down_rx_err").value_or(0) + a.delta("up_rx_err").value_or(0));
    l.unsent = static_cast<int>(a.delta("down_tx_err").value_or(0) + b.delta("up_tx_err").value_or(0));
    return l;
}

}   // namespace


void LinkPath::Source::add(Values v) {
    double t = now();
    if(!hist.empty() && v.contains("uptime") && hist.back().second.contains("uptime") &&
       v["uptime"] < hist.back().second["uptime"]) {
        hist.clear();   // the node restarted (or its uptime wrapped after 18 h)
    }
    hist.emplace_back(t, v);
    count += 1;
    // one report older than the window kept: slow reports (DATALINK_REPORT every 5 s) still give a rate
    while(hist.size() > 2 && t - hist[1].first > WINDOW) {
        hist.pop_front();
    }
}

std::optional<double> LinkPath::Source::period() const {
    if(hist.size() < 2) {
        return std::nullopt;
    }
    return (hist.back().first - hist.front().first) / (hist.size() - 1);
}

double LinkPath::Source::age() const {
    return hist.empty() ? 1e9 : now() - hist.back().first;
}

bool LinkPath::Source::fresh() const {
    // a report in the last 3 s, or in the last 2.5 report periods for the slow ones
    return !hist.empty() && age() < std::max(3.0, 2.5 * period().value_or(5.0));
}

std::optional<double> LinkPath::Source::delta(QString key) const {
    if(hist.size() < 2 || !hist.front().second.contains(key) || !hist.back().second.contains(key)) {
        return std::nullopt;
    }
    if(wrap.contains(key)) {
        // summed per report: a fast counter (bytes) can wrap more than once over the window
        double m = std::pow(2.0, wrap[key]);
        double sum = 0;
        std::optional<double> prev;
        for(auto &h: hist) {
            if(!h.second.contains(key)) {
                continue;
            }
            double v = h.second[key];
            if(prev) {
                sum += std::fmod(std::fmod(v - *prev, m) + m, m);
            }
            prev = v;
        }
        return sum;
    }
    double d = hist.back().second[key] - hist.front().second[key];
    return d >= 0 ? std::optional<double>(d) : std::nullopt;
}

std::optional<double> LinkPath::Source::rate(QString key) const {
    auto d = delta(key);
    double dt = hist.size() < 2 ? 0 : hist.back().first - hist.front().first;
    if(!d || dt <= 0) {
        return std::nullopt;
    }
    return *d / dt;
}

std::optional<double> LinkPath::Source::reportRate() const {
    double dt = hist.size() < 2 ? 0 : hist.back().first - hist.front().first;
    if(dt <= 0) {
        return std::nullopt;
    }
    return (hist.size() - 1) / dt;
}

QList<double> LinkPath::Source::times(QString key) const {
    QList<double> ts;
    for(auto &h: hist) {
        if(h.second.contains(key)) {
            ts.append(h.first);
        }
    }
    return ts;
}

std::optional<double> LinkPath::Source::counterAt(QString key, double t) const {
    // unwrapped, linear between two reports (none outside the reports)
    QList<QPair<double, double>> pts;
    double off = 0;
    std::optional<double> prev;
    for(auto &h: hist) {
        if(!h.second.contains(key)) {
            continue;
        }
        double v = h.second[key];
        if(prev && wrap.contains(key) && v < *prev) {
            off += std::pow(2.0, wrap[key]);
        }
        prev = v;
        pts.append({h.first, v + off});
    }
    for(int i = 0; i + 1 < pts.size(); i++) {
        auto [t0, c0] = pts[i];
        auto [t1, c1] = pts[i + 1];
        if(t0 <= t && t <= t1) {
            return t1 == t0 ? c1 : c0 + (c1 - c0) * (t - t0) / (t1 - t0);
        }
    }
    return std::nullopt;
}

std::optional<double> LinkPath::Source::max(QString key) const {
    // the largest value over the window (not the report kept from before it)
    double t = now();
    std::optional<double> m;
    for(auto &h: hist) {
        if(h.second.contains(key) && t - h.first <= WINDOW) {
            m = std::max(m.value_or(h.second[key]), h.second[key]);
        }
    }
    return m;
}

std::optional<double> LinkPath::Source::last(QString key) const {
    if(hist.empty() || !hist.back().second.contains(key)) {
        return std::nullopt;
    }
    return hist.back().second[key];
}


LinkPath::LinkPath(QString ac_id, QObject *parent) : QObject(parent),
    ac_id(ac_id), ap(WRAP_DATALINK)
{
    auto dispatcher = PprzDispatcher::get();
    dispatcher->bindOptional("DATALINK_REPORT", this, [=,this](QString sender, pprzlink::Message msg) {
        if(sender == this->ac_id) {
            ap.add(fields(msg, DATALINK_REPORT));
        }
    });
    dispatcher->bindOptional("LINK_STATS", this, [=,this](QString sender, pprzlink::Message msg) {
        if(sender != this->ac_id) {
            return;
        }
        auto v = fields(msg, LINK_STATS);
        if(!v.contains("hop")) {
            return;
        }
        int hop = static_cast<int>(v["hop"]);
        if(!stats.contains(hop)) {
            stats.insert(hop, Source(WRAP_STATS));
        }
        stats[hop].add(v);
        emit nodeStats();
    });
    dispatcher->bindOptional("LINK_REPORT", this, [=,this](QString sender, pprzlink::Message msg) {
        (void)sender;
        QString id, link_id;
        msg.getField("ac_id", id);
        if(id != this->ac_id) {
            return;
        }
        msg.getField("link_id", link_id);
        links[link_id].add(fields(msg, LINK_REPORT));
    });
}

bool LinkPath::hasPath() {
    if(ap.fresh()) {
        return true;
    }
    for(auto &s: stats) {
        if(s.fresh()) {
            return true;
        }
    }
    return false;
}

QList<LinkPath::Leg> LinkPath::legs() {
    // the ends along the path: the autopilot, the nodes by hop, the links on this computer
    for(auto &hop: stats.keys()) {
        if(stats[hop].age() > NODE_TIMEOUT) {
            stats.remove(hop);
        }
    }
    QList<End> ends;
    if(stats.contains(0)) {
        ends.append({nodeName(0), &stats[0], End::NODE, stats[0].fresh()});
    } else {
        ends.append({nodeName(0), &ap, End::AP, ap.fresh()});
    }
    for(auto it = stats.begin(); it != stats.end(); ++it) {
        if(it.key() > 0) {
            ends.append({nodeName(it.key()), &it.value(), End::NODE, it.value().fresh()});
        }
    }

    QList<Leg> ret;
    for(int i = 0; i + 1 < ends.size(); i++) {
        ret.append(leg(ends[i], ends[i + 1], false));
    }
    QStringList ids;
    for(auto it = links.begin(); it != links.end(); ++it) {
        if(it.value().age() < NODE_TIMEOUT) {
            ids.append(it.key());
        }
    }
    for(auto &id: ids) {
        QString name = ids.size() == 1 || id == "no_id" ? QString("GCS") : id;
        ret.append(leg(ends.last(), {name, &links[id], End::LINK, links[id].fresh()}, ids.size() > 1));
    }
    return ret;
}

QList<LinkPath::Node> LinkPath::nodes() {
    QList<Node> ret;
    for(auto it = stats.begin(); it != stats.end(); ++it) {
        auto &s = it.value();
        if(!s.fresh()) {
            continue;
        }
        Node n;
        n.hop = it.key();
        auto bat = s.last("battery");
        if(bat && static_cast<int>(*bat) != NO_BATTERY) {
            n.battery = static_cast<int>(*bat);
        }
        auto temp = s.last("temperature");
        if(temp && static_cast<int>(*temp) != UNKNOWN) {
            n.temperature = static_cast<int>(*temp);
        }
        n.peers = static_cast<int>(s.last("nb_peers").value_or(0));
        n.dropped = static_cast<int>(s.delta("dropped").value_or(0));
        ret.append(n);
    }
    return ret;
}

QString LinkPath::missing() {
    if(hasPath() || stats.contains(0)) {
        return QString();
    }
    int reports = 0;
    for(auto &l: links) {
        reports = std::max(reports, l.count);
    }
    if(ap.count == 0 && reports >= 15) {    // link reports every second, DATALINK_REPORT every 5 s or so
        return "No DATALINK_REPORT from the aircraft (in its telemetry?)";
    }
    return QString();
}
