#include "settings_explorer.h"
#include "ui_settings_explorer.h"
#include "dispatcher_ui.h"
#include "AircraftManager.h"
#include <QPainter>
#include <QPaintEvent>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <QScrollArea>
#include <QGroupBox>
#include <QRadioButton>
#include <switch.h>
#include <QToolButton>

SettingsExplorer::SettingsExplorer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsExplorer)
{
    ui->setupUi(this);
    ui->stackedWidget->removeWidget(ui->page);
    ui->stackedWidget->removeWidget(ui->page_2);

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &SettingsExplorer::handleNewAC);
    connect(ui->aircraft_combo, qOverload<int>(&QComboBox::currentIndexChanged),
        [=](int ix) {
            this->repaint();
            DispatcherUi::get()->ac_selected(ac_ids[ix]);
        });

    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this,
        [=](QString ac_id) {
        for(auto ids: ac_ids) {
            if(ids.second == ac_id) {
                ui->aircraft_combo->setCurrentIndex(ids.first);
                ui->stackedWidget->setCurrentIndex(ids.first);
            }
        }
    });

    connect(PprzDispatcher::get(), &PprzDispatcher::dl_values, this, &SettingsExplorer::updateSettings);
}

SettingsExplorer::~SettingsExplorer()
{
    delete ui;
}

void SettingsExplorer::handleNewAC(QString ac_id) {
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    ui->aircraft_combo->addItem(ac.name());
    ac_ids[ui->aircraft_combo->count()-1] = ac_id;

    auto settings = AircraftManager::get()->getAircraft(ac_id).getSettingMenu();
    QTabWidget* pageWidget = new QTabWidget(this);

    populateTab(pageWidget, settings, ac_id);
    ui->stackedWidget->addWidget(pageWidget);
    ui->stackedWidget->setCurrentWidget(pageWidget);


}

void SettingsExplorer::populateTab(QTabWidget* tab, shared_ptr<SettingMenu> settings, QString ac_id) {

    for(auto sets: settings->getSettingMenus()) {
        if(sets->getSettingMenus().size() > 0) {
            QTabWidget* tab_insise = new QTabWidget();
            tab->addTab(tab_insise, sets->getName().c_str());
            populateTab(tab_insise, sets, ac_id);
        } else {
            QWidget* widget = new QWidget();
            QGridLayout* l = new QGridLayout(widget);
            for(auto set: sets->getSettings()) {
                fillSetting(set, ac_id, l, widget);
            }

            QScrollArea* scroll = new QScrollArea();
            scroll->setWidget(widget);
            scroll->setWidgetResizable(true);
            tab->addTab(scroll, sets->getName().c_str());
        }

    }
}

void SettingsExplorer::fillSetting(shared_ptr<Setting> setting, QString ac_id, QGridLayout* lay, QWidget* parent) {
    QWidget* label = new QLabel(setting->getName().c_str(), parent);
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lay->addWidget(label, lay->rowCount(), 0);

    QPushButton* value_btn = new QPushButton("?", parent);
    value_btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lay->addWidget(value_btn, lay->rowCount()-1, 1);
    auto ok_btn = new QToolButton(parent);
    ok_btn->setText(QString::fromUtf8("\xE2\x9C\x93"));
    ok_btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lay->addWidget(ok_btn, lay->rowCount()-1, 3);

    auto undo_btn = new QToolButton(parent);
    undo_btn->setText(QString::fromUtf8("\xE2\x86\xA9"));
    undo_btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lay->addWidget(undo_btn, lay->rowCount()-1, 4);

    connect(value_btn, &QPushButton::clicked, [=]() {
        qDebug() << "setting " << setting->getNo() << " of AC " << ac_id << " clicked !";
        value_btn->setText("?");
        pprzlink::Message getSetting(PprzDispatcher::get()->getDict()->getDefinition("GET_DL_SETTING"));
        getSetting.addField("ac_id", ac_id.toStdString());
        getSetting.addField("index", setting->getNo());
        PprzDispatcher::get()->sendMessage(getSetting);
    });

    connect(undo_btn, &QToolButton::clicked, [=]() {
        pprzlink::Message dlSetting(PprzDispatcher::get()->getDict()->getDefinition("DL_SETTING"));
        dlSetting.addField("ac_id", ac_id.toStdString());
        dlSetting.addField("index", setting->getNo());
        dlSetting.addField("value", setting->getPreviousValue());
        PprzDispatcher::get()->sendMessage(dlSetting);
    });

    initialized[setting] = false;

    auto [min, max, step] = setting->getBounds();

    if(setting->getValues().size() > 2) {
        // named values -> combo box
        QComboBox* combo = new QComboBox(parent);
        for(string s:setting->getValues()) {
            combo->addItem(s.c_str());
        }
        lay->addWidget(combo, lay->rowCount()-1, 2);

        setters[setting] = [combo](double value) {combo->setCurrentIndex(static_cast<int>(value));};
        label_setters[setting] = [combo, value_btn](double value) {value_btn->setText(combo->itemText(static_cast<int>(value)));};

        connect(ok_btn, &QToolButton::clicked, [=]() {
            float value = static_cast<float>(combo->currentIndex());
            setting->setValue(value);
            pprzlink::Message dlSetting(PprzDispatcher::get()->getDict()->getDefinition("DL_SETTING"));
            dlSetting.addField("ac_id", ac_id.toStdString());
            dlSetting.addField("index", setting->getNo());
            dlSetting.addField("value", value);
            PprzDispatcher::get()->sendMessage(dlSetting);
        });

    } else if(setting->getValues().size() == 2 || abs(min+step-max) < 0.0001) {
        QString s1 = setting->getValues().size() == 2 ? setting->getValues()[0].c_str() : QString::number(min);
        QString s2 = setting->getValues().size() == 2 ? setting->getValues()[1].c_str() : QString::number(max);
        QHBoxLayout *hbox = new QHBoxLayout;
        Switch* sw = new Switch();
        QLabel* l1 = new QLabel(s1);
        l1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        QLabel* l2 = new QLabel(s2);
        l2->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(l1);
        hbox->addWidget(sw);
        hbox->addWidget(l2);

        lay->addLayout(hbox, lay->rowCount()-1, 2);


        setters[setting] = [sw](double value) {sw->setChecked(value > 0.5);};
        label_setters[setting] = [setting, value_btn, s1, s2](double value) {
            QString txt;
            if(setting->getValues().size() == 2) {
                txt = value < 0.5 ? s1: s2;
            } else {
                txt = QString::number(value);
            }

            value_btn->setText(txt);
        };

        connect(ok_btn, &QToolButton::clicked, [=]() {
            float value = static_cast<float>(sw->isChecked());
            setting->setValue(value);
            pprzlink::Message dlSetting(PprzDispatcher::get()->getDict()->getDefinition("DL_SETTING"));
            dlSetting.addField("ac_id", ac_id.toStdString());
            dlSetting.addField("index", setting->getNo());

            qDebug() << sw->isChecked() << " " << setting->getNo();
            dlSetting.addField("value", value); //TODO
            PprzDispatcher::get()->sendMessage(dlSetting);
        });

    } else if(abs(max-min) < 0.00001) {
        setters[setting] = [](double value) {(void)value;};
        label_setters[setting] = [value_btn](double value) {value_btn->setText(QString::number(value));};
        float value = min;
        QLabel* uniq_val = new QLabel(QString::number(value));
        uniq_val->setAlignment(Qt::AlignCenter);
        lay->addWidget(uniq_val, lay->rowCount()-1, 2);

        connect(ok_btn, &QToolButton::clicked, [=]() {
            setting->setValue(value);
            pprzlink::Message dlSetting(PprzDispatcher::get()->getDict()->getDefinition("DL_SETTING"));
            dlSetting.addField("ac_id", ac_id.toStdString());
            dlSetting.addField("index", setting->getNo());
            dlSetting.addField("value", value);
            PprzDispatcher::get()->sendMessage(dlSetting);
        });

    } else {
        DoubleSlider* slider = new DoubleSlider(Qt::Horizontal, parent);

        setters[setting] = [slider](double value) {slider->setDoubleValue(value);};
        label_setters[setting] = [value_btn](double value) {value_btn->setText(QString::number(value));};


        slider->setDoubleRange(min, max, step);
        QVBoxLayout* vbox = new QVBoxLayout();
        QLabel* la = new QLabel(QString::number(min, 'f', 2));
        int precision = 0;
        if(step < 1) {
            precision = static_cast<int>(ceil(abs(log10(step))));
        }
        connect(slider, &DoubleSlider::doubleValueChanged,
            [=](double value) {
                la->setText(QString::number(value, 'f', precision));
            });
        vbox->addWidget(la);
        la->setAlignment(Qt::AlignCenter);
        vbox->addWidget(slider);
        lay->addLayout(vbox, lay->rowCount()-1, 2);

        connect(ok_btn, &QToolButton::clicked, [=]() {
            float value = static_cast<float>(slider->doubleValue());
            setting->setValue(value);
            pprzlink::Message dlSetting(PprzDispatcher::get()->getDict()->getDefinition("DL_SETTING"));
            dlSetting.addField("ac_id", ac_id.toStdString());
            dlSetting.addField("index", setting->getNo());
            dlSetting.addField("value", value);
            PprzDispatcher::get()->sendMessage(dlSetting);
        });
    }
}

void SettingsExplorer::paintEvent(QPaintEvent *event) {
    if(ui->aircraft_combo->count() > 0) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRect(event->rect());
        p.setPen(Qt::NoPen);

        QColor color = AircraftManager::get()->getAircraft(ac_ids[ui->aircraft_combo->currentIndex()]).getColor();
        int hue = color.hue();
        int sat = color.saturation();
        color.setHsv(hue, static_cast<int>(sat*0.5), 255);

        p.fillPath(path, color);
        p.drawPath(path);
    }

    QWidget::paintEvent(event);
}

void SettingsExplorer::updateSettings(pprzlink::Message msg) {
    std::string ac_id;
    std::string values;
    msg.getField("ac_id", ac_id);
    msg.getField("values", values);
    QString id = QString(ac_id.c_str());

    auto settings = AircraftManager::get()->getAircraft(id).getSettingMenu()->getAllSettings();
    sort(settings.begin(), settings.end(),
        [](shared_ptr<Setting> sl, shared_ptr<Setting> sr) {
                return sl->getNo() < sr->getNo();
    });

    std::stringstream ss(values);
    std::string token;
    size_t i=0;
    while (std::getline(ss, token, ',')) {
        if(token != "?") {
            double s = stod(token);
            (void)s;
            assert(settings[i]->getNo() == static_cast<uint8_t>(i));
            label_setters[settings[i]](s);
            if(!initialized[settings[i]]) {
                //initialize last values.
                settings[i]->setValue(s);
                settings[i]->setValue(s);
                setters[settings[i]](s);
                initialized[settings[i]] = true;
            }
        }
        i++;
    }
}
