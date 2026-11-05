#ifndef LINKPATH_H
#define LINKPATH_H

#include <QtCore>
#include <deque>
#include <optional>
#include "pprzlinkQt/Message.h"

/// Where frames get lost between an aircraft and the GCS, as sw/misc/link/link_stats.py of Paparazzi:
/// from DATALINK_REPORT (the autopilot), LINK_STATS (each node on the path, by its hop from the autopilot)
/// and LINK_REPORT (link on this computer), per leg between two neighbours of the path, the loss each way.
class LinkPath : public QObject
{
    Q_OBJECT
public:
    struct Leg {
        QString name;
        std::optional<double> down_loss;    // %
        std::optional<double> up_loss;      // %
        std::optional<double> max_gap;      // ms, the longest time without a downlink frame
        std::optional<int> dbm;             // signal of the leg (at its receiving end)
        int rejected = 0;                   // frames with a bad checksum or length at the receiving ends
        int unsent = 0;                     // frames the sending ends could not send
    };

    /// a node on the path (e.g. a Herelink air unit or handheld)
    struct Node {
        int hop;
        std::optional<int> battery;         // %
        std::optional<int> temperature;     // deg C
        int peers = 0;                      // GCS computers it found
        int dropped = 0;                    // frames not forwarded on purpose, over the window
    };

    explicit LinkPath(QString ac_id, QObject *parent = nullptr);

    bool hasPath();         // reports from the aircraft side: the legs can be shown
    QList<Leg> legs();
    QList<Node> nodes();    // the nodes heard lately
    QString missing();      // what is missing for the legs, if something

    static QString nodeName(int hop) {return hop == 0 ? QString("AP") : QString::number(hop);}

    using Values = QHash<QString, double>;

    /// History of the counters of one report source (autopilot, a node on the path, link)
    class Source {
    public:
        Source(QHash<QString, int> wrap = {}): wrap(wrap) {}
        void add(Values v);
        std::optional<double> period() const;
        double age() const;
        bool fresh() const;
        std::optional<double> delta(QString key) const;
        std::optional<double> rate(QString key) const;
        std::optional<double> reportRate() const;
        QList<double> times(QString key) const;
        std::optional<double> counterAt(QString key, double t) const;
        std::optional<double> max(QString key) const;
        std::optional<double> last(QString key) const;
        int count = 0;
    private:
        QHash<QString, int> wrap;      // counters that wrap (number of bits)
        std::deque<std::pair<double, Values>> hist;
    };

signals:
    void nodeStats();       // a LINK_STATS of a node was received

private:
    QString ac_id;
    Source ap;
    QMap<int, Source> stats;            // LINK_STATS by hop
    QMap<QString, Source> links;        // LINK_REPORT by link id
};

#endif // LINKPATH_H
