#include "grid_item.h"
#include <QBrush>
#include <QPen>

#include "mapwidget.h"

GridItem::GridItem(QString id, float xmin, float ymin, float cell_w, float cell_h, int rows, int cols, QObject* parent)
    : MapItem(id, 15, parent),
      xmin(xmin), ymin(ymin), cell_w(cell_w), cell_h(cell_h), rows(rows), cols(cols), grid_map(nullptr)
{
    ac_id = id;
    update_origin();
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("Rows and cols must be positive");
    }
    if (cell_w <= 0 || cell_h <= 0) {
        throw std::invalid_argument("Cell dimensions must be positive");
    }
    cells.resize(rows);
    for(int r = 0; r < rows; ++r) {
        cells[r].resize(cols);
        for(int c = 0; c < cols; ++c) {
            auto rect = new QGraphicsRectItem(0, 0, cell_w, cell_h);
            rect->setPen(Qt::NoPen);
            rect->setBrush(Qt::white);
            cells[r][c] = rect;
        }
    }
}

void GridItem::addToMap(MapWidget* map) {
    for(auto& row : cells)
        for(auto* cell : row)
            map->scene()->addItem(cell);
}

void GridItem::updateGraphics(MapWidget* map, uint32_t update_event) {
    if(!grid_map) return;
    for(int r = 0; r < rows; ++r) {
        for(int c = 0; c < cols; ++c) {
            // Calcula el centro de la celda en coordenadas relativas
            float rel_x = xmin + c * cell_w + cell_w/2;
            float rel_y = ymin + r * cell_h + cell_h/2;

            // Esquina superior izquierda de la celda
            float rel_x1 = xmin + c * cell_w;
            float rel_y1 = ymin + r * cell_h;

            // Esquina inferior derecha
            float rel_x2 = rel_x1 + cell_w;
            float rel_y2 = rel_y1 + cell_h;

            // Convertir a lat/lon
            Point2DLatLon cell_center = CoordinatesTransform::get()->relative_utm_to_wgs84(ltp_origin, rel_x, rel_y);
            Point2DLatLon corner1 = CoordinatesTransform::get()->relative_utm_to_wgs84(ltp_origin, rel_x1, rel_y1);
            Point2DLatLon corner2 = CoordinatesTransform::get()->relative_utm_to_wgs84(ltp_origin, rel_x2, rel_y2);

            // Convertir a escena
            QPointF scene_center = scenePoint(cell_center, zoomLevel(map->zoom()), map->tileSize());
            QPointF scene1 = scenePoint(corner1, zoomLevel(map->zoom()), map->tileSize());
            QPointF scene2 = scenePoint(corner2, zoomLevel(map->zoom()), map->tileSize());

            // Calcular ancho y alto en píxeles
            float width = std::abs(scene2.x() - scene1.x());
            float height = std::abs(scene2.y() - scene1.y());
            
            // Colocar el rectángulo en la posición correcta
            cells[r][c]->setRect(
                scene_center.x() - width/2,
                scene_center.y() - height/2,
                width*0.95,
                height*0.95
            );


            // Color según valor
            if((grid_map->value(r, c)) > 100) {
                cells[r][c]->setBrush(QBrush(Qt::red));
            }
            else if((grid_map->value(r, c)) < -100) {
                cells[r][c]->setBrush(QBrush(Qt::green));
            } 
            else {
                cells[r][c]->setBrush(QBrush(Qt::gray));
            }
        }
    }
}


void GridItem::setVisible(bool vis) {
    for(auto& row : cells) {
        for(auto* cell : row) {
            cell->setVisible(vis);
        }
    }
}


void GridItem::removeFromScene(MapWidget* map) {
    for(auto& row : cells)
        for(auto* cell : row) {
            map->scene()->removeItem(cell);
            delete cell;
        }
}

void GridItem::update_origin() {
    auto ac = pprzApp()->toolbox()->aircraftManager()->getAircraft(ac_id);
    auto origin = ac->getFlightPlan()->getOrigin();

    if(origin) {
    ltp_origin = Point2DLatLon(origin->getLat(), origin->getLon());
    } else {
        qDebug() << "Grid: Can't get origin waypoint from AC " << ac_id;
    }
}

void GridItem::setHighlighted(bool h) {
    MapItem::setHighlighted(h);
    // Puedes cambiar el color de las celdas si quieres destacar el grid
}

void GridItem::setForbidHighlight(bool fh) {
    // No se usa en GridItem, pero debe implementarse
    (void)fh;
}

void GridItem::setEditable(bool ed) {
    // No se usa en GridItem, pero debe implementarse
    (void)ed;
}

void GridItem::updateZValue() {
    // Actualiza el z-value de todas las celdas
    for (auto& row : cells)
        for (auto* cell : row)
            cell->setZValue(z_value);
}