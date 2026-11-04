#ifndef PLOTTER_H
#define PLOTTER_H

#include <QtWidgets>
#include "pprz_dispatcher.h"
#include "configurable.h"

/// A line of a plot: the values of a series (see Plotter::record), drawn in a color
struct PlotLine {
    QString series;     // the key of its values in the plotter, e.g. "ground:ENGINE_STATUS:bat"
    QString name;       // shown before its value when the plot has several lines ("ground")
    QColor color;
    bool dashed = false;
    int decimals = 1;
    // its own fixed range (e.g. the throttle 0..100 % with the current): not in the common scale
    bool own_range = false;
    double range_min = 0, range_max = 100;
};

/// A plot: one or more lines, a unit, dashed level lines (e.g. low battery)
struct PlotDef {
    QString id;         // a preset ("battery", ...) or the series of a message field
    QString title;
    QString unit;
    QList<PlotLine> lines;
    QList<std::tuple<double, QString, QColor>> levels;     // value, name, color
};

/**
 * The drawing of a plot: its lines over the time window, the level lines, the min and max of the
 * scale, and the moment under the mouse (shared by all the plots of the panel).
 */
class PlotChart : public QWidget
{
    Q_OBJECT
public:
    PlotChart(class Plotter* plotter, PlotDef def, QWidget* parent = nullptr);
    QSize sizeHint() const override { return QSize(360, 140); }
    QSize minimumSizeHint() const override { return QSize(200, 90); }

signals:
    void hovered(qint64 time);     // ms since epoch; -1: left

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void leaveEvent(QEvent*) override;

private:
    double timeAt(double x) const;
    class Plotter* plotter;
    PlotDef def;
};

/**
 * Plots of the selected aircraft: one to start (the battery), more with "+ Add plot" below it.
 * The title of a plot switches it (the presets: battery, current and throttle, energy used,
 * speeds, height; or any field of a ground message), ✕ removes it. One time window for all (1, 5
 * or 15 min) and, with the mouse over a plot, the values of all of them at that moment.
 * The presets are recorded from the start: a plot added later has its history.
 */
class Plotter : public QWidget, public Configurable
{
    Q_OBJECT
public:
    explicit Plotter(QString ac_id, QWidget *parent = nullptr);

    void configure(QDomElement c) override;

    struct Point { qint64 t; double v; };
    const QList<Point>& values(const QString& series) { return data[series]; }
    qint64 window() const { return window_ms; }
    qint64 cursor() const { return cursor_time; }
    /// the value of a series at a time (the last one before it), or none
    std::optional<double> valueAt(const QString& series, qint64 t);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    PlotDef preset(QString id);
    QStringList presetIds() const { return {"battery", "current", "power", "energy", "speed", "height", "attitude"}; }
    bool window_is_preset() const { return window_ms == 60000 || window_ms == 300000 || window_ms == 900000; }
    PlotDef fieldPlot(QString series, int index = -1);
    void addPlot(QString id, int at = -1);
    void setPlot(int row, QString id);
    void removePlot(int row);
    void rebuildRows();
    void updateValues();
    void setWindow(qint64 ms);
    void save();
    void record(QString series, double v, qint64 t, int level = 0);
    void listen(QString series, int index = -1);
    QMenu* plotMenu(std::function<void(QString)> chosen);

    QString ac_id;
    QMap<QString, QList<Point>> data;       // the values of each series, the last 15 min
    QSet<QString> listened;                 // the series of message fields recorded
    QMap<QString, double> scales;           // of the message fields (layout or dropped)
    double last_alt = 0, last_agl = 0;      // for the target height (target_alt is MSL)
    bool has_alt = false;
    QHash<QString, int> best_level;         // the fastest source received for each series

    QStringList plot_ids;                   // the plots shown, top to bottom
    struct Row { QWidget* widget; QToolButton* title; QLabel* value; PlotChart* chart; PlotDef def; };
    QList<Row> rows;

    qint64 window_ms = 5 * 60 * 1000;
    qint64 cursor_time = -1;
    QLabel* status;
    QLabel* cursor_label;
    QPushButton* custom_button = nullptr;
    QList<QPushButton*> window_buttons;
    QVBoxLayout* rows_layout;
    QTimer* refresh;
    bool configured = false;
};

#endif // PLOTTER_H
