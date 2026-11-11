#include "attitude_3d.h"
#include "AircraftManager.h"
#include "gcs_utils.h"
#include "pprz_dispatcher.h"
#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <cmath>

namespace {

const double QUAT_FRAC = 1 << 15;       // INT32_QUAT_FRAC
const double ANGLE_BFP = 1. / (1 << 12);    // INT32_ANGLE_FRAC: rad
const double CAMERA_DISTANCE = 5;
const double GROUND = 1.5;              // m below the aircraft
const int GRID = 4;
const int STALE_MS = 3000;

const QColor SETPOINT(0xf0a020);        // the accent orange, as in the ATC panel
const QColor FRONT(0xd92d20);
const QColor STRUCTURE(0x9aa4ae);

using Vec3 = Attitude3D::Vec3;

Vec3 add(const Vec3 &a, const Vec3 &b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
Vec3 sub(const Vec3 &a, const Vec3 &b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
Vec3 scale(const Vec3 &a, double s) { return {a[0] * s, a[1] * s, a[2] * s}; }
double dot(const Vec3 &a, const Vec3 &b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
Vec3 cross(const Vec3 &a, const Vec3 &b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
Vec3 normalized(const Vec3 &a) {
    double n = std::sqrt(dot(a, a));
    return n > 1e-9 ? scale(a, 1 / n) : a;
}

/// Camera looking at the origin from its direction (azimuth) and elevation (looking down)
struct Camera {
    Vec3 forward, right, up, position;
    QPointF center;
    double focal;

    Camera(double azimuth, double elevation, QRectF area) {
        Vec3 h = {std::cos(azimuth), std::sin(azimuth), 0};
        forward = add(scale(h, std::cos(elevation)), Vec3{0, 0, std::sin(elevation)});
        right = {-std::sin(azimuth), std::cos(azimuth), 0};
        up = cross(right, forward);
        position = scale(forward, -CAMERA_DISTANCE);
        center = area.center();
        focal = std::min(area.width(), area.height()) * 1.15;
    }
    double depth(const Vec3 &p) const { return dot(sub(p, position), forward); }
    QPointF project(const Vec3 &p) const {
        Vec3 v = sub(p, position);
        double z = std::max(dot(v, forward), 0.1);
        return center + QPointF(dot(v, right), -dot(v, up)) * (focal / z);
    }
};

/// 6 faces of a box, from its center and half sizes
void addBox(std::vector<std::pair<std::vector<Vec3>, QColor>> &faces, Vec3 c, Vec3 h, QColor color) {
    auto p = [&](int sx, int sy, int sz) { return Vec3{c[0] + sx * h[0], c[1] + sy * h[1], c[2] + sz * h[2]}; };
    // corners counterclockwise seen from outside
    faces.push_back({{p(1, -1, -1), p(1, 1, -1), p(1, 1, 1), p(1, -1, 1)}, color});       // front
    faces.push_back({{p(-1, -1, -1), p(-1, -1, 1), p(-1, 1, 1), p(-1, 1, -1)}, color});   // back
    faces.push_back({{p(-1, 1, -1), p(-1, 1, 1), p(1, 1, 1), p(1, 1, -1)}, color});       // right
    faces.push_back({{p(-1, -1, -1), p(1, -1, -1), p(1, -1, 1), p(-1, -1, 1)}, color});   // left
    faces.push_back({{p(-1, -1, -1), p(-1, 1, -1), p(1, 1, -1), p(1, -1, -1)}, color});   // top
    faces.push_back({{p(-1, -1, 1), p(1, -1, 1), p(1, 1, 1), p(-1, 1, 1)}, color});       // bottom
}

/// A flat disc (rotor) in the x-y plane, both sides
void addDisc(std::vector<std::pair<std::vector<Vec3>, QColor>> &faces, Vec3 c, double r, QColor color) {
    std::vector<Vec3> top, bottom;
    for(int i = 0; i < 12; i++) {
        double a = i * M_PI / 6;
        top.push_back({c[0] + r * std::cos(a), c[1] + r * std::sin(a), c[2]});
    }
    bottom.assign(top.rbegin(), top.rend());
    faces.push_back({bottom, color});
    faces.push_back({top, color});
}

QString modelName(Attitude3D::Model m) {
    switch(m) {
    case Attitude3D::PLANE: return "Fixedwing";
    case Attitude3D::TAILSITTER: return "Tail-sitter (nose up in a hover)";
    default: return "Quadrotor";
    }
}

}   // namespace


Attitude3D::Attitude3D(QString ac_id, QWidget *parent) : QWidget(parent),
    ac_id(ac_id)
{
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    model = QUAD;
    if(ac->getAirframe() != nullptr && ac->getAirframe()->getFirmware() == "fixedwing") {
        model = PLANE;
    }
    auto settings = getAppSettings();
    model = static_cast<Model>(settings.value("attitude3d/model/" + ac->name(), model).toInt());
    follow = settings.value("attitude3d/follow", true).toBool();

    PprzDispatcher::get()->bindOptional("AHRS_REF_QUAT", this, [=, this](QString sender, pprzlink::Message msg) {
        if(sender != this->ac_id) {
            return;
        }
        int32_t q[8];
        const char* names[] = {"ref_qi", "ref_qx", "ref_qy", "ref_qz", "body_qi", "body_qx", "body_qy", "body_qz"};
        for(int i = 0; i < 8; i++) {
            msg.getField(names[i], q[i]);
        }
        setpoint = Quat{q[0] / QUAT_FRAC, q[1] / QUAT_FRAC, q[2] / QUAT_FRAC, q[3] / QUAT_FRAC};
        setEstimate({q[4] / QUAT_FRAC, q[5] / QUAT_FRAC, q[6] / QUAT_FRAC, q[7] / QUAT_FRAC}, true);
    });
    PprzDispatcher::get()->bindOptional("ATTITUDE", this, [=, this](QString sender, pprzlink::Message msg) {
        if(sender != this->ac_id) {
            return;
        }
        float phi, theta, psi;
        msg.getField("phi", phi);
        msg.getField("theta", theta);
        msg.getField("psi", psi);
        setEstimate(quatOfEulers(phi, theta, psi), false);
    });
    PprzDispatcher::get()->bindOptional("ROTORCRAFT_FP", this, [=, this](QString sender, pprzlink::Message msg) {
        if(sender != this->ac_id) {
            return;
        }
        int32_t phi, theta, psi;
        msg.getField("phi", phi);
        msg.getField("theta", theta);
        msg.getField("psi", psi);
        setEstimate(quatOfEulers(phi * ANGLE_BFP, theta * ANGLE_BFP, psi * ANGLE_BFP), false);
    });
    setToolTip("Solid: estimated attitude, outline: setpoint.\nDrag: turn the view; right click: model and view.");
}

QSize Attitude3D::sizeHint() const
{
    return QSize(300, 260);
}

QSize Attitude3D::minimumSizeHint() const
{
    return QSize(200, 180);
}

void Attitude3D::setEstimate(const Quat &q, bool from_ref_msg)
{
    if(from_ref_msg) {
        has_ref_msg = true;
    } else if(has_ref_msg && last_update.isValid() && last_update.elapsed() < STALE_MS) {
        return;     // AHRS_REF_QUAT is there: the estimate goes with its setpoint
    }
    if(!from_ref_msg) {
        setpoint.reset();
    }
    estimate = q;
    last_update.start();
    update();
}

Attitude3D::Vec3 Attitude3D::bodyToNed(const Quat &q, const Vec3 &v)
{
    // R: NED to body (as float_rmat_of_quat); NED = R^T body
    const double qi = q[0], qx = q[1], qy = q[2], qz = q[3];
    const double r[3][3] = {
        {2 * (qi * qi + qx * qx) - 1, 2 * (qx * qy + qi * qz), 2 * (qx * qz - qi * qy)},
        {2 * (qx * qy - qi * qz), 2 * (qi * qi + qy * qy) - 1, 2 * (qy * qz + qi * qx)},
        {2 * (qx * qz + qi * qy), 2 * (qy * qz - qi * qx), 2 * (qi * qi + qz * qz) - 1},
    };
    Vec3 n{};
    for(int i = 0; i < 3; i++) {
        n[i] = r[0][i] * v[0] + r[1][i] * v[1] + r[2][i] * v[2];
    }
    return n;
}

double Attitude3D::angleBetween(const Quat &a, const Quat &b)
{
    double na = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2] + a[3] * a[3]);
    double nb = std::sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2] + b[3] * b[3]);
    if(na < 1e-9 || nb < 1e-9) {
        return 0;
    }
    double d = std::abs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]) / (na * nb);
    return 2 * std::acos(std::min(1.0, d)) * 180 / M_PI;
}

Attitude3D::Quat Attitude3D::quatOfEulers(double phi, double theta, double psi)
{
    double cp = std::cos(phi / 2), sp = std::sin(phi / 2);
    double ct = std::cos(theta / 2), st = std::sin(theta / 2);
    double cs = std::cos(psi / 2), ss = std::sin(psi / 2);
    return {cp * ct * cs + sp * st * ss,
            sp * ct * cs - cp * st * ss,
            cp * st * cs + sp * ct * ss,
            cp * ct * ss - sp * st * cs};
}

/// The faces in the model frame (x forward, y right, z down), sizes in m (not to scale)
std::vector<Attitude3D::Face> Attitude3D::modelFaces() const
{
    QColor ac_color(0x2f7fd1);
    if(AircraftManager::get()->aircraftExists(ac_id)) {
        ac_color = AircraftManager::get()->getAircraft(ac_id)->getColor();
    }
    std::vector<std::pair<std::vector<Vec3>, QColor>> f;
    if(model == QUAD) {
        addBox(f, {0, 0, 0}, {0.35, 0.25, 0.1}, ac_color);
        for(int i = 0; i < 4; i++) {
            double a = M_PI / 4 + i * M_PI / 2;     // front right, back right, back left, front left
            Vec3 tip = {1.0 * std::cos(a), 1.0 * std::sin(a), 0};
            // an arm as a thin box along its diagonal: two segments, each a box
            for(double s = 0.25; s < 1.0; s += 0.25) {
                addBox(f, {tip[0] * s, tip[1] * s, 0}, {0.13, 0.13, 0.04}, STRUCTURE);
            }
            bool front = tip[0] > 0;
            addDisc(f, {tip[0], tip[1], -0.1}, 0.42, front ? FRONT : STRUCTURE);
        }
    } else {
        addBox(f, {0, 0, 0}, {1.1, 0.12, 0.12}, STRUCTURE);                 // fuselage
        addBox(f, {1.15, 0, 0}, {0.1, 0.1, 0.1}, FRONT);                    // nose
        addBox(f, {0.15, 0, 0}, {0.3, 1.6, 0.03}, ac_color);                // wing
        addBox(f, {-0.95, 0, 0}, {0.15, 0.55, 0.02}, ac_color);             // horizontal tail
        addBox(f, {-0.95, 0, -0.28}, {0.15, 0.02, 0.26}, ac_color);         // fin, up
    }
    std::vector<Face> faces;
    for(auto &[points, color] : f) {
        Face face{points, color};
        if(model == TAILSITTER) {
            // nose (model x) along -z body, belly (model z) along +x body
            for(auto &p : face.points) {
                p = {p[2], p[1], -p[0]};
            }
        }
        faces.push_back(face);
    }
    return faces;
}

double Attitude3D::modelHeading(const Quat &q) const
{
    Vec3 f;
    if(model == TAILSITTER) {
        // the nose in forward flight, the belly in a hover (both: the horizontal part)
        Vec3 nose = bodyToNed(q, {0, 0, -1});
        Vec3 belly = bodyToNed(q, {1, 0, 0});
        f = {nose[0] + belly[0], nose[1] + belly[1], 0};
    } else {
        f = bodyToNed(q, {1, 0, 0});
    }
    if(std::hypot(f[0], f[1]) < 1e-3) {
        return azimuth;
    }
    return std::atan2(f[1], f[0]);
}

void Attitude3D::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QColor text = palette().color(QPalette::WindowText);
    QColor secondary = text;
    secondary.setAlpha(170);

    bool stale = !estimate || !last_update.isValid() || last_update.elapsed() > STALE_MS;
    if(follow && estimate) {
        azimuth = modelHeading(*estimate);
    }
    QRectF area = QRectF(rect()).adjusted(0, 28, 0, -18);
    Camera cam(azimuth, elevation, area);

    // ground grid and the directions
    QColor grid = text;
    grid.setAlpha(45);
    p.setPen(QPen(grid, 1));
    for(int i = -GRID; i <= GRID; i++) {
        p.drawLine(cam.project({double(i), double(-GRID), GROUND}), cam.project({double(i), double(GRID), GROUND}));
        p.drawLine(cam.project({double(-GRID), double(i), GROUND}), cam.project({double(GRID), double(i), GROUND}));
    }
    QFont f = font();
    f.setPixelSize(12);
    f.setBold(true);
    p.setFont(f);
    p.setPen(secondary);
    const std::pair<const char*, Vec3> labels[] = {
        {"N", {GRID + 0.6, 0, GROUND}}, {"S", {-GRID - 0.6, 0, GROUND}},
        {"E", {0, GRID + 0.6, GROUND}}, {"W", {0, -GRID - 0.6, GROUND}}};
    for(auto &[l, pos] : labels) {
        if(cam.depth(pos) > 0.5) {
            p.drawText(QRectF(cam.project(pos) - QPointF(10, 8), QSizeF(20, 16)), Qt::AlignCenter, l);
        }
    }

    auto faces = modelFaces();
    // the estimate: solid faces, far ones first, lit from above
    if(estimate) {
        struct Drawn { QPolygonF poly; double depth; QColor color; };
        std::vector<Drawn> drawn;
        const Vec3 light = normalized({-0.3, 0.4, -1});     // to the light, from above
        for(auto &face : faces) {
            std::vector<Vec3> w;
            for(auto &pt : face.points) {
                w.push_back(bodyToNed(*estimate, pt));
            }
            Vec3 n = normalized(cross(sub(w[1], w[0]), sub(w[2], w[0])));
            Vec3 c{};
            for(auto &pt : w) {
                c = add(c, scale(pt, 1. / w.size()));
            }
            if(dot(n, sub(c, cam.position)) >= 0) {
                continue;   // facing away
            }
            double lum = 0.55 + 0.45 * std::max(0.0, dot(n, light));
            QColor color = face.color;
            color = QColor::fromRgbF(color.redF() * lum, color.greenF() * lum, color.blueF() * lum);
            if(stale) {
                color.setAlpha(90);
            }
            QPolygonF poly;
            for(auto &pt : w) {
                poly << cam.project(pt);
            }
            drawn.push_back({poly, cam.depth(c), color});
        }
        std::sort(drawn.begin(), drawn.end(), [](auto &a, auto &b) { return a.depth > b.depth; });
        for(auto &d : drawn) {
            p.setPen(QPen(d.color.darker(130), 0.8));
            p.setBrush(d.color);
            p.drawPolygon(d.poly);
        }
    }

    // the setpoint: the outline of its faces that face the camera
    if(setpoint && !stale) {
        QColor sc = SETPOINT;
        sc.setAlpha(200);
        p.setPen(QPen(sc, 1.4));
        p.setBrush(Qt::NoBrush);
        for(auto &face : faces) {
            std::vector<Vec3> w;
            for(auto &pt : face.points) {
                w.push_back(bodyToNed(*setpoint, pt));
            }
            Vec3 n = cross(sub(w[1], w[0]), sub(w[2], w[0]));
            if(dot(n, sub(w[0], cam.position)) >= 0) {
                continue;
            }
            QPolygonF poly;
            for(auto &pt : w) {
                poly << cam.project(pt);
            }
            p.drawPolygon(poly);
        }
    }

    // the attitude error, or what is missing
    f.setPixelSize(13);
    p.setFont(f);
    QString status;
    QColor status_color = secondary;
    if(stale) {
        status = estimate ? "No recent attitude data" : "No attitude data";
    } else if(setpoint) {
        double err = angleBetween(*estimate, *setpoint);
        status = QString("Attitude error %1°").arg(qRound(err));
        status_color = err > 30 ? FRONT : err > 10 ? SETPOINT : QColor(0x1f7a3f);
    } else {
        status = "No setpoint (AHRS_REF_QUAT)";
    }
    p.setPen(status_color);
    p.drawText(QRectF(4, 4, width() - 8, 20), Qt::AlignLeft | Qt::AlignVCenter, status);

    // legend
    f.setPixelSize(11);
    f.setBold(false);
    p.setFont(f);
    p.setPen(secondary);
    QString legend = setpoint ? "Solid: estimate   Outline: setpoint" : "Estimate";
    legend += follow ? "   View: behind" : "   View: dragged";
    p.drawText(QRectF(4, height() - 18, width() - 8, 16), Qt::AlignLeft | Qt::AlignVCenter, legend);
}

void Attitude3D::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        drag_from = event->position();
        return;
    }
    QWidget::mousePressEvent(event);
}

void Attitude3D::mouseMoveEvent(QMouseEvent *event)
{
    if(drag_from) {
        QPointF d = event->position() - *drag_from;
        drag_from = event->position();
        follow = false;
        azimuth -= d.x() * 0.01;
        elevation = std::clamp(elevation + d.y() * 0.01, -0.3, 1.5);
        update();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void Attitude3D::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        drag_from.reset();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void Attitude3D::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    auto models = menu.addMenu("Model");
    for(auto m : {QUAD, PLANE, TAILSITTER}) {
        auto a = models->addAction(modelName(m));
        a->setCheckable(true);
        a->setChecked(model == m);
        connect(a, &QAction::triggered, this, [=, this]() {
            model = m;
            if(AircraftManager::get()->aircraftExists(ac_id)) {
                getAppSettings().setValue("attitude3d/model/" + AircraftManager::get()->getAircraft(ac_id)->name(), static_cast<int>(m));
            }
            update();
        });
    }
    auto behind = menu.addAction("View from behind (follows the heading)");
    behind->setCheckable(true);
    behind->setChecked(follow);
    connect(behind, &QAction::triggered, this, [this](bool checked) {
        follow = checked;
        if(follow) {
            elevation = 0.35;
        }
        getAppSettings().setValue("attitude3d/follow", follow);
        update();
    });
    menu.exec(event->globalPos());
}
