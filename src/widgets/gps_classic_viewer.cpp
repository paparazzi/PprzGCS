#include "gps_classic_viewer.h"
#include "AircraftManager.h"
#include <QDebug>

GPSClassicViewer::GPSClassicViewer(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id), reduced(true)
{
    connect(AircraftManager::get()->getAircraft(ac_id).getStatus(),
            &AircraftStatus::engine_status, [=]() {
        this->update();
    });
}

void GPSClassicViewer::mousePressEvent(QMouseEvent *event) {
    (void)event;
    reduced = !reduced;
}

void GPSClassicViewer::paintEvent(QPaintEvent *event) {
    (void)event;
    QPainter p(this);

    auto [pacc, infos] = getData();

    int nb_sat_used = 0;
    for(auto info: infos) {
        if(info.flags & 0x01) {
            nb_sat_used++;
        }
    }

    if(pacc > 800 || nb_sat_used <= 4) {
        p.setBrush(QColor("#ff8888"));
    } else if(pacc > 400 || nb_sat_used <= 6) {
        p.setBrush(QColor("#ffc088"));
    } else {
        p.setBrush(QColor("#88ff88"));
    }
    p.setPen(Qt::NoPen);
    p.drawRect(rect());

    int ypacc = 0;

    if(!reduced) {
        minSize = QSize(infos.size() * SV_WIDTH, 20 + 90 + 20 + 20 + 20);
        setMinimumSize(minSize);

        const int yId = 20 + 90;
        ypacc = yId + 20;

        for(int i=0; i<infos.size(); i++) {
            int id = infos[i].id;
            int flags = infos[i].flags;
            int cno = infos[i].cno;
            int age = infos[i].age;

            p.setPen(Qt::black);

            auto rect_cno = QRect(SV_WIDTH*i, 0, SV_WIDTH, 20);
            p.drawText(rect_cno, Qt::AlignCenter, QString::number(cno), &rect_cno);


            p.setPen(Qt::NoPen);
            if(age > 4) {
                p.setBrush(QColor("#888888"));
            } else {
                if(flags & 0x01) {
                    p.setBrush(QColor("#00a000"));
                } else {
                    p.setBrush(QColor("#cc0000"));
                }
            }

            int s = (qMax(20, cno) - 20) * 3;

            int bar_top = yId - s;
            auto rect_cno_bar = QRect(SV_WIDTH*i, bar_top, SV_WIDTH, s);

            p.drawRect(rect_cno_bar.marginsRemoved(QMargins(5, 5, 5, 5)));


            p.setPen(Qt::black);
            auto rect_id = QRect(SV_WIDTH*i, yId, SV_WIDTH, 20);
            p.drawText(rect_id, Qt::AlignCenter, QString::number(id), &rect_id);
        }
    } else {
        minSize = QSize(200, 40);
        setMinimumSize(minSize);
    }


    auto rect_acc = QRect(0, ypacc, rect().width(), 20);
    auto txt = QString("Pos accuracy: %1m").arg(pacc/100., 0, 'f', 1);
    QFont f;
    f.setBold(true);
    p.setFont(f);
    if(pacc > 2000) {
        p.setPen(Qt::red);
    } else {
        p.setPen(Qt::black);
    }
    p.drawText(rect_acc, Qt::AlignLeft, txt, &rect_acc);


    auto rect_nb_used = QRect(0, ypacc + 20, rect().width(), 20);
    p.drawText(rect_nb_used, Qt::AlignLeft, QString("%1 satellites used").arg(nb_sat_used), &rect_nb_used);





}

QSize GPSClassicViewer::sizeHint() const
{
    return minimumSizeHint();
}

QSize GPSClassicViewer::minimumSizeHint() const
{
    return QSize(qMax(minSize.width(), 200), qMax(minSize.height(), 40));
}


std::tuple<uint16_t, QList<struct GPSClassicViewer::SvInfo>> GPSClassicViewer::getData() {
    auto msg = AircraftManager::get()->getAircraft(ac_id).getStatus()->getMessage("SVSINFO");
    if(!msg) {
        return make_tuple(65535, QList<struct SvInfo>());
    }

    uint16_t pacc;
    std::string _svids, _flags, _qis, _cnos, _elevs, _azims, _msg_ages;

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

    for(auto id: QString(_svids.c_str()).split(',')) {
        svids.append(id.toInt());
    }

//    for(auto sv: QString(_svids.c_str()).split(',')) {
//        svids.append(sv.toInt());
//    }

    for(auto fls: QString(_flags.c_str()).split(',')) {
        flags.append(fls.toInt());
    }

    // qis

    for(auto cn: QString(_cnos.c_str()).split(',')) {
        cno.append(cn.toInt());
    }

    // elevs

    // azims

    for(auto age: QString(_msg_ages.c_str()).split(',')) {
        msg_age.append(age.toInt());
    }


    for(int i=0; i<svids.size(); i++) {
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
