#include "gps_classic_viewer.h"
#include "AircraftManager.h"
#include <QDebug>

GPSClassicViewer::GPSClassicViewer(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id)
{
    connect(AircraftManager::get()->getAircraft(ac_id)->getStatus(),
            &AircraftStatus::engine_status, this, [=,this]() {
        this->update();
    });
    connect(AircraftManager::get()->getAircraft(ac_id)->getStatus(),
            &AircraftStatus::svsinfo, this, [=,this]() {
        this->update();
    });
}

/**
 * The position accuracy and the number of satellites used, with the quality as a colored bar
 * and text; below, a signal bar per satellite (green: used, gray: not used, faded: old).
 */
void GPSClassicViewer::paintEvent(QPaintEvent *event) {
    (void)event;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto [pacc, infos] = getData();
    bool no_data = pacc == 65535;
    int nb_sat_used = 0;
    for(auto &info: infos) {
        if(info.flags & 0x01) {
            nb_sat_used++;
        }
    }

    QColor quality, quality_bg;
    QString quality_txt;
    if(no_data) {
        quality = QColor(0x6e7781); quality_bg = QColor(0xeef1f4); quality_txt = "No data";
    } else if(pacc > 800 || nb_sat_used <= 4) {
        quality = QColor(0xb42318); quality_bg = QColor(0xfde8e6); quality_txt = "Poor";
    } else if(pacc > 400 || nb_sat_used <= 6) {
        quality = QColor(0x8a4b00); quality_bg = QColor(0xfff4e0); quality_txt = "Fair";
    } else {
        quality = QColor(0x1f7a3f); quality_bg = QColor(0xe3f1e6); quality_txt = "Good";
    }
    QColor text = palette().color(QPalette::WindowText);
    QColor secondary = text;
    secondary.setAlpha(170);

    // first row: accuracy, satellites, quality pill
    QFont caption = font();
    caption.setPixelSize(11);
    QFont value = font();
    value.setPixelSize(18);
    value.setBold(true);

    p.setFont(caption);
    p.setPen(secondary);
    p.drawText(QRectF(0, 0, 120, 16), Qt::AlignLeft | Qt::AlignVCenter, "Accuracy");
    p.drawText(QRectF(110, 0, 90, 16), Qt::AlignLeft | Qt::AlignVCenter, "Satellites");
    p.setFont(value);
    p.setPen(text);
    p.drawText(QRectF(0, 16, 120, 26), Qt::AlignLeft | Qt::AlignVCenter,
               no_data ? QString::fromUtf8("\xE2\x80\x94") : QString("%1 m").arg(pacc / 100., 0, 'f', 1));
    p.drawText(QRectF(110, 16, 90, 26), Qt::AlignLeft | Qt::AlignVCenter,
               no_data ? QString::fromUtf8("\xE2\x80\x94") : QString("%1 / %2").arg(nb_sat_used).arg(infos.size()));

    QFont pill_font = font();
    pill_font.setPixelSize(12);
    pill_font.setBold(true);
    p.setFont(pill_font);
    QRectF pill(qMax(205.0, width() - 72.0), 12, 70, 24);
    p.setPen(Qt::NoPen);
    p.setBrush(quality_bg);
    p.drawRoundedRect(pill, 12, 12);
    p.setPen(quality);
    p.drawText(pill, Qt::AlignCenter, quality_txt);

    // satellites: signal bars (C/N0), the id below
    const int top = 52, bars_h = 70, label_h = 16;
    int n = infos.size();
    if(n == 0) {
        p.setFont(caption);
        p.setPen(secondary);
        p.drawText(QRectF(0, top, width(), 20), Qt::AlignLeft | Qt::AlignVCenter, "No satellites");
        minSize = QSize(280, top + 24);
    } else {
        double w = qMin(22.0, (width() - 2.0) / n);
        p.setPen(QPen(QColor(128, 128, 128, 60), 1));
        p.drawLine(QPointF(0, top + bars_h), QPointF(width(), top + bars_h));
        p.setFont(caption);
        for(int i=0; i<n; i++) {
            auto &sv = infos[i];
            double h = qBound(2.0, (sv.cno - 15) * bars_h / 35.0, qMax(2.0, static_cast<double>(bars_h)));
            QColor c = (sv.flags & 0x01) ? QColor(0x1f7a3f) : QColor(0x8c959f);
            if(sv.age > 4) {
                c.setAlpha(90);
            }
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawRoundedRect(QRectF(i * w + 2, top + bars_h - h, w - 4, h), 2, 2);
            p.setPen(secondary);
            p.drawText(QRectF(i * w, top + bars_h + 2, w, label_h), Qt::AlignCenter, QString::number(sv.id));
        }
        minSize = QSize(280, top + bars_h + label_h + 4);
    }
    // the quality as a thin line at the bottom
    p.setPen(Qt::NoPen);
    p.setBrush(quality);
    p.drawRoundedRect(QRectF(0, minSize.height() - 3, width(), 3), 1.5, 1.5);
    if(minimumSize() != minSize) {
        setMinimumSize(minSize);
    }
}

QSize GPSClassicViewer::sizeHint() const
{
    return minimumSizeHint();
}

QSize GPSClassicViewer::minimumSizeHint() const
{
    return QSize(qMax(minSize.width(), 280), qMax(minSize.height(), 76));
}


std::tuple<uint16_t, QList<struct GPSClassicViewer::SvInfo>> GPSClassicViewer::getData() {
    auto msg = AircraftManager::get()->getAircraft(ac_id)->getStatus()->getMessage("SVSINFO");
    if(!msg) {
        return make_tuple(65535, QList<struct SvInfo>());
    }

    uint16_t pacc;
    QString _svids, _flags, _qis, _cnos, _elevs, _azims, _msg_ages;

    msg->getField("pacc", pacc);
    msg->getField("svid", _svids);
    msg->getField("flags", _flags);
    //msg->getField("qi", _qis);
    msg->getField("cno", _cnos);
    //msg->getField("elev", _elevs);
    //msg->getField("azim", _azims);
    msg->getField("msg_age", _msg_ages);

    QList<int> svids;
    QList<int> flags;
    //QList<int> qis;
    QList<int> cno;
    //QList<int> elevs;
    //QList<int> azims;
    QList<int> msg_age;

    QList< struct SvInfo> infos;

    for(auto &id: _svids.split(',')) {
        svids.append(id.toInt());
    }

//    for(auto sv: QString(_svids.c_str()).split(',')) {
//        svids.append(sv.toInt());
//    }

    for(auto &fls: _flags.split(',')) {
        flags.append(fls.toInt());
    }

    // qis

    for(auto &cn: _cnos.split(',')) {
        cno.append(cn.toInt());
    }

    // elevs

    // azims

    for(auto &age: _msg_ages.split(',')) {
        msg_age.append(age.toInt());
    }


    // the lists should have the same length; do not read past a shorter one
    int n = qMin(qMin(svids.size(), flags.size()), qMin(cno.size(), msg_age.size()));
    for(int i=0; i<n; i++) {
        infos.append(SvInfo{svids[i], flags[i], cno[i], msg_age[i]});
    }

    QMutableListIterator<struct SvInfo> i(infos);
    while (i.hasNext()) {
        auto next = i.next();
        if (next.age > 60 || next.id == 0) {
            i.remove();
        }
    }

    return make_tuple(pacc, infos);
}
