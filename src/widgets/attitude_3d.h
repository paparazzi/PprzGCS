#ifndef ATTITUDE_3D_H
#define ATTITUDE_3D_H

#include <QWidget>
#include <QElapsedTimer>
#include <array>
#include <optional>

/**
 * 3D attitude of an aircraft, after the attitude_viz tool of Paparazzi: the estimated attitude as
 * a solid model and the attitude setpoint as an outline (AHRS_REF_QUAT), above a ground grid with
 * north, east, south and west. Shows at a glance what pitch and roll angles hide, e.g. for a
 * tail-sitter, and whether the aircraft follows its setpoint (the attitude error).
 * Without AHRS_REF_QUAT the estimate comes from ATTITUDE or ROTORCRAFT_FP (no setpoint).
 * Drag: turn the view around; right click: model (quadrotor, fixedwing, tail-sitter) and view.
 */
class Attitude3D : public QWidget
{
    Q_OBJECT
public:
    explicit Attitude3D(QString ac_id, QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    enum Model {
        QUAD,
        PLANE,
        TAILSITTER,     // a plane with its nose along -z body: up in a hover (level body frame)
    };

    using Vec3 = std::array<double, 3>;
    using Quat = std::array<double, 4>;     // qi, qx, qy, qz: NED to body

    /// Body vector in NED
    static Vec3 bodyToNed(const Quat &q, const Vec3 &v);
    /// Angle of the rotation between two attitudes (deg)
    static double angleBetween(const Quat &a, const Quat &b);
    static Quat quatOfEulers(double phi, double theta, double psi);

protected:
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    struct Face {
        std::vector<Vec3> points;   // in the model frame: x forward, y right, z down
        QColor color;
    };

    void setEstimate(const Quat &q, bool from_ref_msg);
    std::vector<Face> modelFaces() const;
    /// heading (rad) of the model, for the view behind it
    double modelHeading(const Quat &q) const;

    QString ac_id;
    Model model;
    std::optional<Quat> estimate;
    std::optional<Quat> setpoint;
    bool has_ref_msg = false;       // AHRS_REF_QUAT received: ATTITUDE is not used
    QElapsedTimer last_update;

    bool follow = true;             // view behind the aircraft, else from where it was dragged
    double azimuth = 0;             // rad: the direction the camera looks to (0: north)
    double elevation = 0.35;        // rad: looking down
    std::optional<QPointF> drag_from;
};

#endif // ATTITUDE_3D_H
