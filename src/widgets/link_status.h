#ifndef LINKSTATUS_H
#define LINKSTATUS_H

#include <QtWidgets>

/// The telemetry links of an aircraft: a compact table, a row per link (its state by the dot,
/// its numbers in columns, the captions once above them).
class LinkStatus : public QWidget
{
    Q_OBJECT
public:
    explicit LinkStatus(QString ac_id, QWidget *parent = nullptr);

private:
    void updateData();
    void updatePath();      // where frames get lost on the way (from the link statistics)

    struct Row {
        QLabel* dot;
        QLabel* name;
        QLabel* ping;
        QLabel* rx;
        QLabel* downlink;
        QLabel* uplink_lost;
        QList<QWidget*> widgets() {return {dot, name, ping, rx, downlink, uplink_lost};}
    };
    Row makeRow(QString link_id);

    QString ac_id;
    QGridLayout* grid;
    QList<QLabel*> captions;
    QLabel* empty_label;
    QMap<QString, Row> rows;
    int next_row = 1;      // row 0: the captions

    // the path below the links, in the same columns
    static constexpr int PATH_ROW = 1000;
    QList<QLabel*> path_captions;
    QList<QList<QLabel*>> path_rows;    // name, down loss, up loss, max gap, signal
    QLabel* path_notes;
};

#endif // LINKSTATUS_H
