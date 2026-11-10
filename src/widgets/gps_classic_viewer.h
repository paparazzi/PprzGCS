#ifndef GPS_CLASSIC_VIEWER_H
#define GPS_CLASSIC_VIEWER_H

#include <QtWidgets>

/**
 * GPS status of an aircraft, from the SVSINFO message of the server:
 *  - header: position accuracy, satellites used / tracked, quality (Good, Fair, Poor) and the
 *    button of the sky plot;
 *  - a signal bar (C/N0) per satellite, with its id below;
 *  - optional sky plot: the satellites by azimuth (north up) and elevation (zenith in the middle,
 *    horizon on the outer circle).
 * Colors: green used in the fix, gray not used, orange not used and no lock (or unusable), faded
 * when old; SBAS satellites as an outline.
 */
class GPSClassicViewer : public QWidget
{
    Q_OBJECT
public:
    explicit GPSClassicViewer(QString ac_id, QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    struct Satellite {
        int id;         // u-blox numbering: GPS 1-32, SBAS 120-158, ...
        int flags;      // bit 0: used in the fix
        int qi;         // quality: 0 idle, 1 searching, 2 acquired, 3 unusable, 4 code lock,
                        // 5-7 code and carrier lock; -1: unknown
        int cno;        // signal, dBHz
        int age;        // of the last data
        int elev;       // whole deg above the horizon: 0 horizon, 90 straight up; outside -90..90: unknown
        int azim;       // deg from north, clockwise

        bool used() const { return flags & 0x01; }
        bool noLock() const { return qi >= 0 && qi < 4; }
        bool sbas() const { return id >= 120 && id <= 158; }
        bool old() const { return age > 4; }
        // also unknown: 0 / 0, sent for an idle channel
        bool positionKnown() const { return elev >= -90 && elev <= 90 && !(elev == 0 && azim == 0); }
    };

    struct GpsData {
        bool valid = false;     // false before the first SVSINFO
        double accuracy = 0;    // m, 3D position
        QList<Satellite> satellites;
        int used() const;
    };

    GpsData getData() const;

    // each draws from y = top and returns the y below what it drew
    double drawHeader(QPainter &p, const GpsData &gps);
    double drawBars(QPainter &p, const GpsData &gps, double top);
    double drawSkyPlot(QPainter &p, const GpsData &gps, double top);

    QString ac_id;
    QSize minSize;          // from the last paint, follows the content

    bool show_sky = false;  // the sky plot below the bars, saved
    QRectF sky_button;      // from the last paint
};

#endif // GPS_CLASSIC_VIEWER_H
