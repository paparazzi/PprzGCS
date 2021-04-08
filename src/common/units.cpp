#include "units.h"
#include <QApplication>
#include "tinyxml2.h"
#include <QSettings>
#include <QDebug>
#include "iostream"

using namespace std;
using namespace tinyxml2;

Units::Units(PprzApplication* app, PprzToolbox* toolbox) : PprzTool(app, toolbox)
{

}

void Units::setToolbox(PprzToolbox* toolbox) {
    PprzTool::setToolbox(toolbox);
    QSettings settings(qApp->property("SETTINGS_PATH").toString(), QSettings::IniFormat);
    auto filename = settings.value("PAPARAZZI_HOME").toString() + "/var/units.xml";

    setlocale(LC_ALL, "C"); // needed for stod() to use '.' as decimal separator instead of ',' (at least in France)
    XMLDocument doc;
    doc.LoadFile(filename.toStdString().c_str());
    if(doc.Error()) {
        cerr << "Error parsing " << filename.toStdString() << ": " << doc.ErrorStr() << endl;
    }
    else {
        XMLElement* units_root = doc.FirstChildElement( "units" );
        for(auto unit=units_root->FirstChildElement(); unit!=nullptr; unit=unit->NextSiblingElement()) {
            auto from = unit->Attribute("from");
            auto to = unit->Attribute("to");
            auto _coef = unit->Attribute("coef");
            // auto _auto = unit->Attribute("auto");
            assert(from != nullptr && to != nullptr && _coef != nullptr);
            auto coef = stof(_coef);
            auto key = make_pair(from, to);
            coefs[key] = coef;
            //qDebug() << "from " << from << " to " << to << " -> " << coef;

        }

    }
}

std::optional<float> Units::getCoef(std::string inputUnit, std::string outputUnit) {
    auto key = make_pair(inputUnit, outputUnit);
    if(coefs.find(key) != coefs.end()) {
        return coefs[key];
    } else {
        return std::nullopt;
    }
}
