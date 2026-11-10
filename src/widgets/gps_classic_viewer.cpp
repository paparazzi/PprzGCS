#include "gps_classic_viewer.h"
#include "AircraftManager.h"
#include "gcs_utils.h"
#include <algorithm>
#include <cmath>

namespace {

const int MIN_WIDTH = 280;

// signal bars: C/N0 from MIN_CNO (a stub) to MIN_CNO + CNO_RANGE (full height)
const double BARS_HEIGHT = 70;
const double BAR_MAX_WIDTH = 22;
const double LABEL_HEIGHT = 16;
const double MIN_CNO = 15;      // dBHz
const double CNO_RANGE = 35;    // dBHz

// sky plot
const double SKY_MIN_DIAMETER = 120;
const double SKY_MAX_DIAMETER = 260;
const double SKY_LABEL = 14;    // room for N, E, S, W around the outer circle

const int MAX_AGE = 60;
const int UNKNOWN_ELEV = 127;   // outside -90..90: the position is not known         // satellites with older data are left out

const QColor USED(0x1f7a3f);
const QColor NOT_USED(0x8c959f);

struct Quality {
    QColor color;
    QColor background;
    QString text;
};

/// Poor: worse than 8 m or 4 satellites or less, Fair: worse than 4 m or 6 satellites or less
Quality quality(bool valid, double accuracy, int used) {
    if(!valid) {
        return {QColor(0x6e7781), QColor(0xeef1f4), "No data"};
    }
    if(accuracy > 8 || used <= 4) {
        return {QColor(0xb42318), QColor(0xfde8e6), "Poor"};
    }
    if(accuracy > 4 || used <= 6) {
        return {QColor(0x8a4b00), QColor(0xfff4e0), "Fair"};
    }
    return {USED, QColor(0xe3f1e6), "Good"};
}

/// Pen and brush of a satellite, in its bar and in the sky plot: green when used, gray when
/// not, faded when old; an outline for an SBAS satellite (returns the inset for the pen).
template<class Satellite>
double setSatelliteStyle(QPainter &p, const Satellite &sv) {
    QColor c = sv.used() ? USED : NOT_USED;
    if(sv.old()) {
        c.setAlpha(90);
    }
    if(sv.sbas()) {
        p.setPen(QPen(c, 1.5));
        p.setBrush(Qt::NoBrush);
        return 0.75;    // the outline inside the shape
    }
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    return 0;
}

QColor withAlpha(QColor c, int alpha) {
    c.setAlpha(alpha);
    return c;
}

QFont pixelFont(const QFont &base, int px, bool bold) {
    QFont f = base;
    f.setPixelSize(px);
    f.setBold(bold);
    return f;
}

}   // namespace


GPSClassicViewer::GPSClassicViewer(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id)
{
    show_sky = getAppSettings().value("gps/sky_plot", false).toBool();
    auto status = AircraftManager::get()->getAircraft(ac_id)->getStatus();
    connect(status, &AircraftStatus::engine_status, this, [this]() { update(); });
    connect(status, &AircraftStatus::svsinfo, this, [this]() { update(); });
}

QSize GPSClassicViewer::sizeHint() const
{
    return minimumSizeHint();
}

QSize GPSClassicViewer::minimumSizeHint() const
{
    return QSize(qMax(minSize.width(), MIN_WIDTH), qMax(minSize.height(), 76));
}

void GPSClassicViewer::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && sky_button.contains(event->position())) {
        show_sky = !show_sky;
        getAppSettings().setValue("gps/sky_plot", show_sky);
        update();
        return;
    }
    QWidget::mousePressEvent(event);
}

void GPSClassicViewer::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const GpsData gps = getData();
    const double height_without_sky = drawBars(p, gps, drawHeader(p, gps)) + 4;
    double bottom = height_without_sky;
    if(show_sky) {
        bottom = drawSkyPlot(p, gps, bottom + 4) + 2;
    }
    const QSize size(MIN_WIDTH, qRound(bottom));

    // the quality as a thin line at the bottom
    p.setPen(Qt::NoPen);
    p.setBrush(quality(gps.valid, gps.accuracy, gps.used()).color);
    p.drawRoundedRect(QRectF(0, size.height() - 3, width(), 3), 1.5, 1.5);

    // the sky plot is not part of the size set by the user for a card on the map rail
    setProperty("rail_extra_height", size.height() - qRound(height_without_sky));
    if(minSize != size) {
        minSize = size;
        setMinimumSize(minSize);
    }
}

/// Accuracy, satellites used / tracked, the quality and the button of the sky plot
double GPSClassicViewer::drawHeader(QPainter &p, const GpsData &gps)
{
    const QColor text = palette().color(QPalette::WindowText);
    const QString none = QString::fromUtf8("\xE2\x80\x94");    // em dash
    const double col2 = 92;     // x of the satellites

    p.setFont(pixelFont(font(), 11, false));
    p.setPen(withAlpha(text, 170));
    p.drawText(QRectF(0, 0, col2, 16), Qt::AlignLeft | Qt::AlignVCenter, "Accuracy");
    p.drawText(QRectF(col2, 0, 90, 16), Qt::AlignLeft | Qt::AlignVCenter, "Satellites");
    p.setFont(pixelFont(font(), 18, true));
    p.setPen(text);
    p.drawText(QRectF(0, 16, col2, 26), Qt::AlignLeft | Qt::AlignVCenter,
               gps.valid ? QString("%1 m").arg(gps.accuracy, 0, 'f', 1) : none);
    p.drawText(QRectF(col2, 16, 90, 26), Qt::AlignLeft | Qt::AlignVCenter,
               gps.valid ? QString("%1 / %2").arg(gps.used()).arg(gps.satellites.size()) : none);

    // quality pill, then the button: both still fit at the minimum width
    Quality q = quality(gps.valid, gps.accuracy, gps.used());
    QRectF pill(qMax(170.0, width() - 98.0), 12, 70, 24);
    p.setPen(Qt::NoPen);
    p.setBrush(q.background);
    p.drawRoundedRect(pill, 12, 12);
    p.setFont(pixelFont(font(), 12, true));
    p.setPen(q.color);
    p.drawText(pill, Qt::AlignCenter, q.text);

    // sky plot button: a chevron, right when hidden, down when shown
    sky_button = QRectF(pill.right() + 4, 12, 24, 24);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(128, 128, 128, show_sky ? 70 : 30));
    p.drawRoundedRect(sky_button, 6, 6);
    p.setPen(QPen(text, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    QPointF c = sky_button.center();
    if(show_sky) {
        p.drawPolyline(QPolygonF({c + QPointF(-5, -2.5), c + QPointF(0, 2.5), c + QPointF(5, -2.5)}));
    } else {
        p.drawPolyline(QPolygonF({c + QPointF(-2.5, -5), c + QPointF(2.5, 0), c + QPointF(-2.5, 5)}));
    }
    return 52;
}

/// A signal bar (C/N0) per satellite, the id below
double GPSClassicViewer::drawBars(QPainter &p, const GpsData &gps, double top)
{
    const QColor secondary = withAlpha(palette().color(QPalette::WindowText), 170);
    p.setFont(pixelFont(font(), 11, false));
    const int n = gps.satellites.size();
    if(n == 0) {
        p.setPen(secondary);
        p.drawText(QRectF(0, top, width(), 20), Qt::AlignLeft | Qt::AlignVCenter, "No satellites");
        return top + 20;
    }

    const double bottom = top + BARS_HEIGHT;
    p.setPen(QPen(QColor(128, 128, 128, 60), 1));
    p.drawLine(QPointF(0, bottom), QPointF(width(), bottom));
    const double w = qMin(BAR_MAX_WIDTH, (width() - 2.0) / n);
    for(int i = 0; i < n; i++) {
        const auto &sv = gps.satellites[i];
        double h = qBound(2.0, (sv.cno - MIN_CNO) * BARS_HEIGHT / CNO_RANGE, BARS_HEIGHT);
        double inset = setSatelliteStyle(p, sv);
        p.drawRoundedRect(QRectF(i * w + 2, bottom - h, w - 4, h).adjusted(inset, inset, -inset, -inset), 2, 2);
        p.setPen(secondary);
        p.drawText(QRectF(i * w, bottom + 2, w, LABEL_HEIGHT), Qt::AlignCenter, QString::number(sv.id));
    }
    return bottom + LABEL_HEIGHT;
}

/// The satellites by azimuth (north up) and elevation: zenith in the middle, horizon on the outer
/// circle (also for a satellite below it), circles every 30 deg; bigger when used
double GPSClassicViewer::drawSkyPlot(QPainter &p, const GpsData &gps, double top)
{
    const QColor text = palette().color(QPalette::WindowText);
    const double d = qBound(SKY_MIN_DIAMETER, width() - 2 * SKY_LABEL, SKY_MAX_DIAMETER);
    const double r = d / 2;
    const QPointF center(width() / 2., top + SKY_LABEL + r);
    // distance from the center for an elevation (deg)
    auto distance = [r](double elev) { return r * (90 - qBound(0.0, elev, 90.0)) / 90; };

    p.setPen(withAlpha(text, 60));
    p.setBrush(Qt::NoBrush);
    for(double elev: {0., 30., 60.}) {
        p.drawEllipse(center, distance(elev), distance(elev));
    }
    p.drawLine(center - QPointF(r, 0), center + QPointF(r, 0));
    p.drawLine(center - QPointF(0, r), center + QPointF(0, r));

    p.setFont(pixelFont(font(), 11, false));
    p.setPen(withAlpha(text, 170));
    for(auto [letter, dx, dy]: {std::tuple{"N", 0., -1.}, {"E", 1., 0.}, {"S", 0., 1.}, {"W", -1., 0.}}) {
        QPointF pos = center + QPointF(dx, dy) * (r + SKY_LABEL / 2);
        p.drawText(QRectF(pos.x() - SKY_LABEL, pos.y() - SKY_LABEL / 2, 2 * SKY_LABEL, SKY_LABEL), Qt::AlignCenter, letter);
    }

    for(const auto &sv: gps.satellites) {
        if(!sv.positionKnown()) {
            continue;
        }
        double az = sv.azim * M_PI / 180;
        QPointF pos = center + QPointF(std::sin(az), -std::cos(az)) * distance(sv.elev);
        double radius = sv.used() ? 6 : 4;
        double inset = setSatelliteStyle(p, sv);
        p.drawEllipse(pos, radius - inset, radius - inset);
        p.setPen(withAlpha(text, 170));
        p.drawText(QRectF(pos.x() + radius, pos.y() - radius - 10, 30, 12), Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(sv.id));
    }
    return top + 2 * SKY_LABEL + d;
}

int GPSClassicViewer::GpsData::used() const
{
    return std::count_if(satellites.begin(), satellites.end(), [](const Satellite &sv) { return sv.used(); });
}

/// The last SVSINFO of the aircraft: one comma separated list per field, an item per channel
GPSClassicViewer::GpsData GPSClassicViewer::getData() const
{
    GpsData gps;
    auto msg = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getMessage("SVSINFO");
    if(!msg) {
        return gps;
    }
    auto list = [&msg](const char* field) {
        QString csv;
        msg->getField(field, csv);
        QList<int> values;
        for(auto &v: csv.split(',', Qt::SkipEmptyParts)) {
            values.append(v.toInt());
        }
        return values;
    };
    uint16_t pacc;
    msg->getField("pacc", pacc);
    gps.valid = true;
    gps.accuracy = pacc / 100.;

    auto ids = list("svid"), flags = list("flags"), cnos = list("cno"), ages = list("msg_age");
    auto elevs = list("elev"), azims = list("azim");
    // the lists should have the same length; do not read past a shorter one
    int n = std::min({ids.size(), flags.size(), cnos.size(), ages.size()});
    for(int i = 0; i < n; i++) {
        Satellite sv{ids[i], flags[i], cnos[i], ages[i],
                     i < elevs.size() ? elevs[i] : UNKNOWN_ELEV, i < azims.size() ? azims[i] : 0};
        if(sv.id != 0 && sv.age <= MAX_AGE) {   // id 0: empty channel
            gps.satellites.append(sv);
        }
    }
    return gps;
}
