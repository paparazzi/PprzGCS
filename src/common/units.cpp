#include "units.h"
#include <QApplication>
#include <QSettings>
#include <QDebug>
#include "iostream"
#include <QtXml>
#include "gcs_utils.h"

using namespace std;

Units::Units(PprzApplication* app, PprzToolbox* toolbox) : PprzTool(app, toolbox)
{

}

void Units::setToolbox(PprzToolbox* toolbox) {
    PprzTool::setToolbox(toolbox);
    auto settings = getAppSettings();
    auto filename = appConfig()->value("PAPARAZZI_HOME").toString() + "/var/units.xml";


    QDomDocument doc;
    QFile f(filename);
    if(!f.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Error while loading unit file");
    }
    doc.setContent(&f);
    f.close();

    auto units_root = doc.firstChildElement("units");
    for(auto unit=units_root.firstChildElement();
        !unit.isNull();
        unit=unit.nextSiblingElement()) {

        assert(unit.hasAttribute("from"));
        assert(unit.hasAttribute("to"));
        assert(unit.hasAttribute("coef"));
        auto from = unit.attribute("from");
        auto to = unit.attribute("to");
        auto _coef = unit.attribute("coef");

        // auto _auto = unit.attribute("auto");
        auto coef = _coef.toDouble();
        QPair key(from, to);
        coefs[key] = coef;
    }


}

std::optional<float> Units::getCoef(QString inputUnit, QString outputUnit) {
    QPair key(inputUnit, outputUnit);
    if(coefs.find(key) != coefs.end()) {
        return coefs[key];
    } else {
        return std::nullopt;
    }
}
