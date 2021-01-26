#ifndef UNITS_H
#define UNITS_H
#include <optional>
#include <map>

using namespace std;

class Units {
public:
    static Units* get() {
        if(singleton == nullptr) {
            singleton = new Units();
        }
        return singleton;
    }

    optional<float> getCoef(string inputUnit, string outputUnit);

private:
    static Units* singleton;
    explicit Units();
    map<pair<string, string>, float> coefs;
};
#endif // UNITS_H
