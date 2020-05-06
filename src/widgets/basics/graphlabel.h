#ifndef GRAPHLABEL_H
#define GRAPHLABEL_H

#include <QWidget>
#include <QTime>

class GraphLabel : public QWidget
{
    Q_OBJECT
public:
    explicit GraphLabel(QWidget *parent = nullptr);
    explicit GraphLabel(double min, double max, QWidget *parent = nullptr);

    void pushData(double value);

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent*) override;
    //void enterEvent(QEvent*) override;

signals:

public slots:

private:
    QBrush brushTop, brushBottom;
    double timespan;  // total time the graph represent (in seconds)
    double min, max;

    QList<std::tuple<QTime, double>> data;
};

#endif // GRAPHLABEL_H
