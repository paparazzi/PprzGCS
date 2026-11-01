#ifndef LINKSTATUS_H
#define LINKSTATUS_H

#include <QtWidgets>

/// The telemetry links of an aircraft: a row per link with its state and numbers.
class LinkStatus : public QWidget
{
    Q_OBJECT
public:
    explicit LinkStatus(QString ac_id, QWidget *parent = nullptr);

private:
    void updateData();

    struct Row {
        QWidget* widget;
        QLabel* dot;
        QLabel* name;
        QLabel* state;
        QLabel* ping;
        QLabel* rx;
        QLabel* downlink;
        QLabel* uplink_lost;
    };
    Row makeRow(QString link_id);

    QString ac_id;
    QVBoxLayout* rows_layout;
    QLabel* empty_label;
    QMap<QString, Row> rows;
};

#endif // LINKSTATUS_H
