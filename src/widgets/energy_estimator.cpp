#include "energy_estimator.h"
#include <algorithm>
#include <cmath>

namespace {

// Cell voltage at rest every 5 % of charge, from empty (0 %) to full (100 %)
const double OCV[EnergyEstimator::CHEMISTRIES][21] = {
    // LiPo
    {3.27, 3.61, 3.69, 3.71, 3.73, 3.75, 3.77, 3.79, 3.80, 3.82, 3.84,
     3.85, 3.87, 3.91, 3.95, 3.98, 4.02, 4.08, 4.11, 4.15, 4.20},
    // LiHV
    {3.30, 3.62, 3.71, 3.74, 3.77, 3.79, 3.81, 3.83, 3.85, 3.87, 3.90,
     3.93, 3.96, 4.00, 4.05, 4.09, 4.14, 4.19, 4.24, 4.29, 4.35},
    // Li-ion (NMC, NCA: 18650, 21700)
    {3.00, 3.30, 3.42, 3.48, 3.53, 3.57, 3.60, 3.63, 3.66, 3.70, 3.73,
     3.76, 3.80, 3.83, 3.87, 3.91, 3.96, 4.01, 4.06, 4.13, 4.20},
    // LiFe (LiFePO4): mean of a C/20 charge and discharge of an A123 cell (CALCE open data)
    {2.48, 3.09, 3.21, 3.23, 3.25, 3.27, 3.28, 3.29, 3.30, 3.305, 3.306,
     3.307, 3.309, 3.312, 3.32, 3.34, 3.345, 3.347, 3.35, 3.36, 3.51},
};
// a full cell just after charging is above the top of the curve
const double FULL_MARGIN[EnergyEstimator::CHEMISTRIES] = {0.05, 0.05, 0.05, 0.15};
const int N = 21;

const size_t MIN_SAMPLES = 5;
const double KEEP = 30;             // s of points kept
const double TREND_WINDOW = 120;    // s, for the time left without power
const double POWER_WINDOW = 30;     // s, the mean power, for the time left
const double TAU_V = 10;            // s, low-pass of the voltage

// SAG
const double LAG = 2;               // s, the changes over this time
const double TAU_X = 10;            // s, the polarization
const double MEMORY = 300;          // s, of the least squares
const double R_PRIOR = 0.01;        // start covariance of R0 and Rp: R starts at 0 and grows only as
                                    // the changes of the load show it (not too high from the first ones)
const int MIN_UPDATES = 10;

// FILTER
const double TAU_FILTER = 300;      // s, of the pull towards SAG
const double NEW_BATTERY = 0.01;    // Ah: the charge used sent went down by more: a new battery
const double NOT_FLOWN = 0.002;     // Ah used: still before the flight
const double ONBOARD_CHECK = 0.2;   // Ah counted here before the count of the aircraft is checked
const double REBOOT_GAP = 5;        // s without a sample: maybe unplugged (a reboot)
const double NEW_BATTERY_FULL = 0.9;    // after a reboot, a voltage at rest this full: a new battery

/// Energy (V * charge fraction) from empty to each point of the table
struct EnergyTable {
    double e[EnergyEstimator::CHEMISTRIES][N];
    EnergyTable() {
        for(int c = 0; c < EnergyEstimator::CHEMISTRIES; c++) {
            e[c][0] = 0;
            for(int i = 1; i < N; i++) {
                e[c][i] = e[c][i - 1] + 0.05 * (OCV[c][i - 1] + OCV[c][i]) / 2;
            }
        }
    }
};
const EnergyTable ENERGY;

}   // namespace


double EnergyEstimator::ocvOfSoc(Chemistry c, double soc)
{
    double x = std::clamp(soc, 0.0, 1.0) * (N - 1);
    int i = std::min(static_cast<int>(x), N - 2);
    return OCV[c][i] + (x - i) * (OCV[c][i + 1] - OCV[c][i]);
}

double EnergyEstimator::socOfOcv(Chemistry c, double v)
{
    if(v <= OCV[c][0]) {
        return 0;
    }
    if(v >= OCV[c][N - 1]) {
        return 1;
    }
    int i = 0;
    while(i < N - 2 && OCV[c][i + 1] < v) {
        i++;
    }
    return (i + (v - OCV[c][i]) / (OCV[c][i + 1] - OCV[c][i])) / (N - 1);
}

double EnergyEstimator::energyOfSoc(Chemistry c, double soc)
{
    double x = std::clamp(soc, 0.0, 1.0) * (N - 1);
    int i = std::min(static_cast<int>(x), N - 2);
    double f = x - i;
    // the trapezoid from point i to the soc
    double v = ocvOfSoc(c, soc);
    double e = ENERGY.e[c][i] + f * 0.05 * (OCV[c][i] + v) / 2;
    return e / ENERGY.e[c][N - 1];
}

double EnergyEstimator::meanOcv(Chemistry c)
{
    return ENERGY.e[c][N - 1];
}

int EnergyEstimator::guessCells(Chemistry c, double voltage)
{
    return std::max(1, static_cast<int>(std::ceil(voltage / (OCV[c][N - 1] + FULL_MARGIN[c]))));
}

void EnergyEstimator::setChemistry(Chemistry c)
{
    if(c != chemistry) {
        chemistry = c;
        used = -1;          // the SoC at the start was on the other curve
        soc_f = -1;
    }
}

void EnergyEstimator::clear()
{
    points.clear();
    history.clear();
    with_current = false;
    onboard_charge = false;
    resetSag();
    used = -1;
    soc_f = -1;
    onboard_ok = true;
}

void EnergyEstimator::resetSag()
{
    for(int i = 0; i < 3; i++) {
        th[i] = 0;
        for(int j = 0; j < 3; j++) {
            P[i][j] = i == j ? (i < 2 ? R_PRIOR : 1) : 0;
        }
    }
    updates = 0;
    // a known R (airframe): V - I.R can correct from the start, and goes on learning from there
    if(with_current && r_start > 0) {
        th[0] = r_start;
        updates = MIN_UPDATES;
    }
    load_mean = 0;
    load_x = points.empty() ? 0 : points.back().load;
    ocv_lp = -1;
    v_lp = -1;
}

bool EnergyEstimator::motorsOn(const Point &pt) const
{
    return pt.throttle > 5 || pt.current > 2;
}

void EnergyEstimator::setStartResistance(double ohm)
{
    r_start = std::max(0.0, ohm);
    if(with_current && r_start > 0 && updates < MIN_UPDATES) {
        th[0] = r_start;
        updates = MIN_UPDATES;
    }
}

bool EnergyEstimator::rKnown() const
{
    return updates >= MIN_UPDATES;
}

double EnergyEstimator::userOcv() const
{
    return v_lp;
}

double EnergyEstimator::sagOcv() const
{
    return rKnown() ? ocv_lp : v_lp;
}

void EnergyEstimator::addSample(const Sample &s)
{
    if(s.voltage < 2) {
        return;
    }
    if(!points.empty() && s.t < points.back().t) {
        clear();
    }
    if(cells <= 0) {
        cells = guessCells(chemistry, s.voltage);  // from the first voltage (at rest, the highest)
        cells_guessed = true;
    } else if(cells_guessed && guessCells(chemistry, s.voltage) > cells) {
        // the first voltage was low (the pack not fully connected yet): guess again and start over
        cells = guessCells(chemistry, s.voltage);
        clear();
    }
    if((s.current >= 0) != with_current) {
        with_current = s.current >= 0;
        resetSag();     // R of the other load
    }
    double gap = points.empty() ? 0 : s.t - points.back().t;
    double dt = std::min(gap, 5.0);
    Point pt;
    pt.t = s.t;
    pt.cell = s.voltage / cells;
    pt.current = s.current;
    pt.throttle = s.throttle;
    pt.load = with_current ? std::max(0.0, s.current) : std::clamp(s.throttle, 0.0, 100.0);
    load_x = points.empty() ? pt.load : load_x + std::min(1.0, dt / TAU_X) * (pt.load - load_x);
    pt.x = load_x;
    points.push_back(pt);
    while(points.front().t < s.t - KEEP) {
        points.pop_front();
    }

    updateSag(pt, dt);
    updateCharge(s, dt, gap > REBOOT_GAP);

    if(points.size() >= MIN_SAMPLES) {
        history.push_back({s.t, estimate().energy});
        while(history.front().first < s.t - TREND_WINDOW) {
            history.pop_front();
        }
    }
}

void EnergyEstimator::updateSag(const Point &pt, double dt)
{
    // the load in flight, for what is a change of it
    if(pt.load > 0) {
        load_mean = load_mean <= 0 ? pt.load : load_mean + 0.01 * (pt.load - load_mean);
    }
    // the point LAG s before
    const Point *old = nullptr;
    for(auto it = points.rbegin(); it != points.rend(); ++it) {
        if(it->t <= pt.t - LAG) {
            old = &*it;
            break;
        }
    }
    if(old != nullptr && pt.t - old->t < 3 * LAG) {
        double ref = std::max(load_mean, 1.0);  // 1 A, or 1 % of throttle
        double dl = pt.load - old->load;
        double dx = pt.x - old->x;
        if(std::abs(dl) > 0.02 * ref || std::abs(dx) > 0.01 * ref) {
            // dV = -R0 dload - Rp dx - drift dt
            double phi[3] = {-dl, -dx, -(pt.t - old->t)};
            double y = pt.cell - old->cell;
            double lambda = std::exp(-std::max(dt, 0.05) / MEMORY);
            double pphi[3], den = lambda, err = y;
            for(int i = 0; i < 3; i++) {
                pphi[i] = 0;
                for(int j = 0; j < 3; j++) {
                    pphi[i] += P[i][j] * phi[j];
                }
                den += phi[i] * pphi[i];
                err -= phi[i] * th[i];
            }
            for(int i = 0; i < 3; i++) {
                th[i] += pphi[i] / den * err;
            }
            for(int i = 0; i < 3; i++) {
                for(int j = 0; j < 3; j++) {
                    P[i][j] = (P[i][j] - pphi[i] * pphi[j] / den) / lambda;
                }
            }
            th[0] = std::max(0.0, th[0]);
            th[1] = std::max(0.0, th[1]);
            updates++;
        }
    }
    // the voltage at rest: + dV of the user while the motors run, + the fitted sag
    motors = motorsOn(pt);
    double a = std::min(1.0, dt / TAU_V);
    double v = pt.cell + (motors ? sag_user : 0);
    v_lp = v_lp < 0 ? v : v_lp + a * (v - v_lp);
    double ocv = pt.cell + SAG_FACTOR * (th[0] * pt.load + th[1] * pt.x);
    ocv_lp = ocv_lp < 0 ? ocv : ocv_lp + a * (ocv - ocv_lp);
}

void EnergyEstimator::updateCharge(const Sample &s, double dt, bool after_gap)
{
    if(s.charge > 0) {
        onboard_charge = true;
    }
    auto start = [&]() {
        soc0 = socOfOcv(chemistry, sagOcv());
        soc_f = soc0;
        used = 0;
        used_onboard = used_own = 0;
        charge_last = std::max(0.0, s.charge);
    };
    if(used < 0) {
        start();
        return;
    }
    // the voltage now, not low-passed: after a new battery the old one is still in v_lp
    auto fromVoltageNow = [&]() {
        if(!motors) {
            v_lp = ocv_lp = points.back().cell;
        }
        start();
    };
    if(!motors) {
        // on the ground: before the first flight, start from the voltage again. After it, only on
        // hard evidence of a new battery (a voltage that looks fuller than the count is not: the
        // discharge curve of the pack can differ from the table): a reboot (no samples for a while)
        // followed by the voltage of a nearly full battery
        if(used < NOT_FLOWN) {
            start();
            return;
        }
        if(after_gap && socOfOcv(chemistry, points.back().cell) >= NEW_BATTERY_FULL) {
            fromVoltageNow();
            return;
        }
    }
    if(onboard_charge) {
        if(s.charge < charge_last - NEW_BATTERY) {
            fromVoltageNow();   // the count started again: a new battery (or a reboot)
            return;
        }
        used_onboard += std::max(0.0, s.charge - charge_last);
        charge_last = std::max(charge_last, s.charge);
    }
    if(with_current && points.size() > 1) {
        used_own += std::max(0.0, points[points.size() - 2].current) * dt / 3600;
    }
    // a count of the aircraft far below the current it sends does not count (wrong unit, not
    // integrated): then count the current here
    if(onboard_charge && with_current && used_own > ONBOARD_CHECK && used_onboard < 0.5 * used_own) {
        onboard_ok = false;
    }
    double now = onboard_charge && onboard_ok ? used_onboard : used_own;
    double d = std::max(0.0, now - used);   // Ah used since the last sample
    used = now;
    // FILTER: the count, pulled towards the SoC of SAG
    if(capacity > 0) {
        soc_f -= d / (capacity / 1000);
        soc_f += (socOfOcv(chemistry, sagOcv()) - soc_f) * std::min(1.0, dt / TAU_FILTER);
        soc_f = std::clamp(soc_f, 0.0, 1.0);
    }
}

EnergyEstimator::Result EnergyEstimator::estimate(Method chosen) const
{
    Result r;
    if(points.size() < MIN_SAMPLES || cells <= 0) {
        return r;
    }
    const auto &last = points.back();
    r.valid = true;
    r.with_current = with_current;
    r.cell = last.cell;
    r.r_known = rKnown();
    r.resistance = th[0] + th[1];
    r.used = used;
    r.load = last.load;
    r.load_x = last.x;
    bool counted = capacity > 0 && used >= 0 && (onboard_charge || with_current);
    r.ready[VOLTAGE] = true;
    r.ready[CHARGE] = counted;
    r.ready[SAG] = true;            // V + dV of the user until R is known
    r.ready[FILTER] = counted;

    Method m = chosen;
    if(!r.ready[m]) {
        m = m == FILTER ? SAG : VOLTAGE;
    }
    r.method = m;
    double fitted = SAG_FACTOR * (th[0] * last.load + th[1] * last.x);
    switch(m) {
    case VOLTAGE:
        r.ocv = userOcv();
        r.soc = socOfOcv(chemistry, r.ocv);
        r.dv = sag_user;
        break;
    case CHARGE:
        r.soc = std::clamp(soc0 - used / (capacity / 1000), 0.0, 1.0);
        r.ocv = ocvOfSoc(chemistry, r.soc);
        r.dv = 0;
        break;
    case SAG:
        r.ocv = sagOcv();
        r.soc = socOfOcv(chemistry, r.ocv);
        r.dv = r.r_known ? fitted : sag_user;
        break;
    case FILTER:
        r.soc = soc_f;
        r.ocv = ocvOfSoc(chemistry, r.soc);
        r.dv = r.r_known ? fitted : sag_user;
        break;
    }
    r.energy = energyOfSoc(chemistry, r.soc);

    // power: the mean of the last seconds
    if(with_current) {
        double p = 0;
        int k = 0;
        for(auto &pt : points) {
            if(pt.t >= last.t - POWER_WINDOW && pt.current >= 0) {
                p += pt.cell * cells * pt.current;
                k++;
            }
        }
        r.power = k > 0 ? p / k : 0;
    }
    // the time to the reserve; below it, to empty
    r.to_empty = r.energy <= reserve;
    const double target = r.to_empty ? 0 : reserve;
    if(capacity > 0) {
        double total = cells * capacity / 1000 * meanOcv(chemistry);
        r.wh_left = total * r.energy;
        if(r.power > 1) {
            r.minutes = std::max(0.0, (r.energy - target) * total / r.power * 60);
        }
    }

    // else (no current or no capacity) the time left from the trend of the energy, while the motors
    // run. With the current and the capacity, only the power counts: no power (on the ground), no
    // time, and a jump of the estimate (e.g. the sag gone at the landing) does not change it.
    bool from_power = with_current && capacity > 0;
    if(!from_power && motors && r.minutes < 0 && history.size() >= 10 && last.t - history.front().first > 60) {
        double st = 0, se = 0, stt = 0, ste = 0;
        int k = history.size();
        for(auto &[t, e] : history) {
            double dt = t - last.t;
            st += dt;
            se += e;
            stt += dt * dt;
            ste += dt * e;
        }
        double den = k * stt - st * st;
        double slope = den > 0 ? (k * ste - st * se) / den : 0;     // per s
        if(slope < -1e-6) {
            r.minutes = std::max(0.0, (r.energy - target) / -slope / 60);
        }
    }
    return r;
}
