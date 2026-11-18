#ifndef ENERGY_ESTIMATOR_H
#define ENERGY_ESTIMATOR_H

#include <deque>

/**
 * Energy left in a battery (ENGINE_STATUS of the server: voltage, current, throttle, charge used).
 *
 * The voltage of a cell at rest (open circuit, OCV) gives its state of charge (SoC) through the
 * discharge curve of its chemistry; under load the voltage sags by dV. Four estimators, from
 * simple to complex:
 * - VOLTAGE: OCV = V + dV, with dV per cell set by the user (only while the motors run). The default
 *   dV is 0: the voltage alone, which reads low under load but never high.
 * - CHARGE: the SoC at the start (from the voltage) minus the charge used (the Ah integrated on
 *   board, else, or when that count stays below half of the current, the current integrated
 *   here) over the capacity.
 * - SAG: OCV = V + 60 % of R * load, R learned: the load is the current (R in Ohm) when it is
 *   measured, else the throttle (R in V per %). R0 and Rp (polarization, 10 s) are fitted by
 *   recursive least squares on the changes of voltage and load over 2 s, which cancels the slow
 *   fall of the OCV, updated only when the load changes. R starts at 0 with a small covariance:
 *   the first changes of the load cannot make it too high at once. Only 60 % is corrected: the estimate
 *   must not show more charge than is left. A voltage reading that follows the throttle (motor
 *   ripple) makes R too high, and then even 60 % can be too much.
 * - FILTER: complementary filter: the charge used of CHARGE, pulled towards the SoC of SAG with a
 *   time constant of 5 min.
 *
 * The energy left is in Wh (the integral of the OCV over the charge), not in Ah: the last
 * percent of charge holds less energy, at a lower voltage. The time left: from the mean power of
 * the last 30 s and the capacity when both are known, else from how fast the energy went down over
 * the last 2 min.
 */
class EnergyEstimator
{
public:
    enum Chemistry {
        LIPO,
        LIHV,
        LIION,
        LIFE,
    };
    static const int CHEMISTRIES = 4;

    enum Method {
        VOLTAGE,    // V + dV set by the user
        CHARGE,     // charge used over the capacity
        SAG,        // V + 60 % of R * load, R learned
        FILTER,     // CHARGE pulled towards SAG
    };
    static const int METHODS = 4;

    struct Sample {
        double t;           // s
        double voltage;     // V, the pack
        double current;     // A, the pack; negative: not measured
        double throttle;    // %
        double charge;      // Ah used, integrated on board; negative: not sent
    };

    struct Result {
        bool valid = false;
        Method method = VOLTAGE;    // the method used (another one when the chosen one cannot run)
        bool ready[METHODS] = {};   // which methods can run
        double cell = 0;        // V, the cell under load (last sample)
        double ocv = 0;         // V, the cell at rest (estimated)
        double dv = 0;          // V per cell, the sag the estimate corrects for now (none: 0)
        double soc = 0;         // charge left, 0..1
        double energy = 0;      // energy left, 0..1 (Wh)
        double resistance = 0;  // R0 + Rp per cell: Ohm with the current, else V per % of throttle
        bool with_current = false;
        bool r_known = false;   // R is fitted
        double used = -1;       // Ah used since the start, -1: unknown
        double load = 0;        // the load now: A with the current, else % of throttle
        double load_x = 0;      // the load low-passed (10 s), the polarization of V - I.R
        double power = 0;       // W, mean of the last seconds; 0: unknown
        double wh_left = -1;    // Wh, -1: unknown (no capacity)
        double minutes = -1;    // to the reserve (below it: to empty), -1: unknown
        bool to_empty = false;  // below the reserve: minutes is the time to 0 %
    };

    void setChemistry(Chemistry c);
    Chemistry getChemistry() const { return chemistry; }
    void setMethod(Method m) { method = m; }
    Method getMethod() const { return method; }
    void setCells(int n) { cells = n; cells_guessed = false; }
    int getCells() const { return cells; }
    void setCapacity(double mah) { capacity = mah; }   // per pack; <= 0: unknown
    double getCapacity() const { return capacity; }
    void setReserve(double r) { reserve = r; }         // 0..1 of the energy
    double getReserve() const { return reserve; }
    void setSag(double v) { sag_user = v; }            // V per cell under load, for VOLTAGE
    /// R per cell (Ohm) to start V - I.R with (e.g. from the airframe), with the current only; 0: from 0.
    /// Used at the next start of the fit (new battery), or at once while R is not known yet.
    void setStartResistance(double ohm);
    double getStartResistance() const { return r_start; }
    double getSag() const { return sag_user; }

    void addSample(const Sample &s);
    void clear();
    Result estimate() const { return estimate(method); }
    /// the estimate of another method than the chosen one (e.g. to draw them all)
    Result estimate(Method chosen) const;

    /// Cell voltage at rest for a state of charge (0..1)
    static double ocvOfSoc(Chemistry c, double soc);
    /// State of charge (0..1) for a cell voltage at rest
    static double socOfOcv(Chemistry c, double v);
    /// Energy left (0..1) for a state of charge
    static double energyOfSoc(Chemistry c, double soc);
    /// Wh per Ah of a cell from full to empty (the mean OCV)
    static double meanOcv(Chemistry c);
    /// Number of cells for a full or partly used pack voltage
    static int guessCells(Chemistry c, double voltage);

    /// the share of the fitted sag that is corrected for: low enough not to overestimate the
    /// charge left on real flights (safety), while still closer than the voltage alone
    static constexpr double SAG_FACTOR = 0.6;

private:
    struct Point {
        double t;
        double cell;        // V per cell
        double load;        // A, else % of throttle
        double x;           // load low-passed (polarization)
        double current;     // A, negative: not measured
        double throttle;    // %
    };

    void updateSag(const Point &pt, double dt);
    void updateCharge(const Sample &s, double dt, bool after_gap);
    void resetSag();
    bool motorsOn(const Point &pt) const;
    double userOcv() const;     // OCV of VOLTAGE
    double sagOcv() const;      // OCV of SAG (of VOLTAGE until R is known)
    bool rKnown() const;

    std::deque<Point> points;       // the last seconds
    std::deque<std::pair<double, double>> history;     // (t, energy), for the trend
    Chemistry chemistry = LIPO;
    Method method = VOLTAGE;
    int cells = 0;          // 0: guessed from the voltage
    bool cells_guessed = false;     // guessed: a higher voltage can raise it
    double capacity = 0;    // mAh
    double reserve = 0.2;
    double sag_user = 0;    // 0: the voltage alone, never optimistic
    double r_start = 0;     // Ohm per cell, the start of R0 with the current; 0: from 0
    bool with_current = false;

    // VOLTAGE and SAG: the cell voltage (+ the fitted sag), low-passed
    double v_lp = -1;
    double ocv_lp = -1;
    bool motors = false;
    // SAG: recursive least squares, theta = [R0, Rp, drift]
    double th[3] = {0, 0, 0};
    double P[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    double load_x = 0;      // the load low-passed
    double load_mean = 0;   // the load in flight, for the threshold of a change
    int updates = 0;

    // CHARGE and FILTER
    bool onboard_charge = false;    // the aircraft sends the charge used
    double charge_last = 0; // Ah, the last charge used sent
    double used = -1;       // Ah used since the start, -1: not started
    double used_onboard = 0;    // Ah, counted by the aircraft since the start
    double used_own = 0;        // Ah, the current counted here since the start
    bool onboard_ok = true;     // false: the count of the aircraft stays far below the current
    double soc0 = 0;        // SoC at the start
    double soc_f = -1;      // FILTER
};

#endif // ENERGY_ESTIMATOR_H
