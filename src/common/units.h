#ifndef UNITS_H
#define UNITS_H
#include <optional>
#include <map>
#include "PprzToolbox.h"
#include "PprzApplication.h"

using namespace std;

class Units : public PprzTool {
public:
    explicit Units(PprzApplication* app, PprzToolbox* toolbox);
    static Units* get() {
        return pprzApp()->toolbox()->units();
    }

    virtual void setToolbox(PprzToolbox* toolbox) override;

    optional<float> getCoef(string inputUnit, string outputUnit);

private:
    map<pair<string, string>, float> coefs;
};
#endif // UNITS_H
