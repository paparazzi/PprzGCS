#include "gvf_trajectory.h"
#include "mapwidget.h"
#include "gcs_utils.h"

GVF_trajectory::GVF_trajectory(uint8_t id, QPointF pos, Point2DLatLon origin)
{
    ac_id = id;
    ac_pos = pos;
    ltp_origin = origin;
}

QList<QPointF> GVF_trajectory::meshGrid(float area, int xpoints_num, int ypoints_num)
{
    QList<QPointF> grid;

    float dx = sqrt(area)/(xpoints_num - 1);
    float dy = sqrt(area)/(ypoints_num - 1);
    for(float x=xy_off.x() - 0.5*sqrt(area); x<=xy_off.x() + 0.5*sqrt(area) + dx/2; x+=dx) {
        for(float y=xy_off.y() - 0.5*sqrt(area); y<=xy_off.y() + 0.5*sqrt(area) + dy/2; y+=dy) {
            grid.append(QPointF(x,y));
        }
    }

    return grid;
}

QuiverItem* GVF_trajectory::getVField() {
    return vector_field;
}

void GVF_trajectory::createTrajItem(QList<QPointF> points) // TODO
{
    (void)points;
    //     auto pi = new PathItem("__NO_AC__", palette);
    //     if(shape == 1) {    // Polygon
    //         pi->setFilled(true);
    //         pi->setClosedPath(true);
    //     }
    //     pi->setZValues(z, z);
    //     for(auto pos: points) {
    //         auto wi = new WaypointItem(pos, "__NO_AC__", palette);
    //         //wcenter->setEditable(false);
    //         //wcenter->setZValues(z, z);
    //         addItem(wi);
    //         wi->setStyle(GraphicsObject::Style::CURRENT_NAV);
    //         pi->addPoint(wi);
    //     }
    //     pi->setText(text);
    //     addItem(pi);
    //     item = pi;

    // }

    // if(item != nullptr && !shape_id.isNull()) {
    //     shapes[shape_id] = item;
    // }
}

void GVF_trajectory::createVFieldItem(QList<QPointF> points, QList<QPointF> vpoints) 
{
    QList<Point2DLatLon> pos;
    QList<Point2DLatLon> vpos;

    for (int i=0; i<points.size(); i++) {
        pos.append(CoordinatesTransform::get()->ltp_to_wgs84(ltp_origin, points[i].x(), points[i].y()));
        vpos.append(CoordinatesTransform::get()->ltp_to_wgs84(pos[i], vpoints[i].x(), vpoints[i].y()));
    }

    vector_field = new QuiverItem(pos, vpos,  QString::number(ac_id));
}