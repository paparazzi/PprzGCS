#include "aircraft.h"
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"
#include <QDebug>

Aircraft::Aircraft(QString id, QColor color, QString icon, QString n, FlightPlan* fp, shared_ptr<SettingMenu> setting_menu, Airframe air, QObject* parent):
    QObject(parent),
    ac_id(id), color(color), icon(icon), _name(n), flight_plan(fp), setting_menu(setting_menu), airframe(air), position(Point2DLatLon(0,0))
{
    flight_plan->setParent(this);
    status = new AircraftStatus(ac_id);
}

Aircraft::~Aircraft()
{
    if(status != nullptr) {
        //status->deleteLater();
    }
}

void Aircraft::setSetting(shared_ptr<Setting> setting, float value) {
    auto coef = setting->getAltUnitCoef();
    if(value < setting->getMin() / coef || value > setting->getMax() / coef) {
        qDebug() << "Warning: send setting value " << value << ", out of bounds for setting " << setting->getName();
    }

    pprzlink::Message dlSetting(PprzDispatcher::get()->getDict()->getDefinition("DL_SETTING"));
    dlSetting.addField("ac_id", ac_id);
    dlSetting.addField("index", setting->getNo());
    dlSetting.addField("value", value);
    PprzDispatcher::get()->sendMessage(dlSetting);
}

void Aircraft::setSetting(uint8_t setting_no, float value) {
    for(auto set: setting_menu->getAllSettings()) {
        if(set->getNo() == setting_no) {
            setSetting(set, value);
            break;
        }
    }
}
