#include "papgetconfig.h"
#include "AircraftManager.h"

PapgetConfig::PapgetConfig(Papget::DataDef datadef, Papget::Params params, QWidget *parent) :
    CardDialog("Papget", AircraftManager::get()->getAircraft(datadef.ac_id)->name(), parent),
    datadef(datadef), params(params)
{
    setAttribute(Qt::WA_DeleteOnClose);
    auto ac = AircraftManager::get()->getAircraft(datadef.ac_id);
    this->params.style = Papget::Style::TEXT;

    // what it shows
    auto what = addText(QString("%1 · %2%3").arg(datadef.msg_name, datadef.field,
                        datadef.scale != 1 ? QString(" × %1").arg(datadef.scale) : QString()));
    what->setStyleSheet("font-family: Monospace;");

    auto row = [=,this](QString name) {
        auto lay = new QHBoxLayout();
        lay->setSpacing(6);
        auto l = new QLabel(name, this);
        l->setObjectName("card_muted");
        l->setFixedWidth(48);
        lay->addWidget(l);
        body()->addLayout(lay);
        return lay;
    };

    // size: S M L XL
    auto size_lay = row("Size");
    auto sizes = QList<QPair<QString, int>>{{"S", 10}, {"M", 14}, {"L", 20}, {"XL", 28}};
    int closest = 0;
    for(int i=0; i<sizes.size(); i++) {
        if(abs(sizes[i].second - params.fontSize) < abs(sizes[closest].second - params.fontSize)) {
            closest = i;
        }
    }
    auto size_group = new QButtonGroup(this);
    for(int i=0; i<sizes.size(); i++) {
        auto b = new QPushButton(sizes[i].first, this);
        b->setCheckable(true);
        b->setChecked(i == closest);
        b->setObjectName("papget_size");
        b->setMinimumHeight(30);
        size_group->addButton(b, i);
        size_lay->addWidget(b, 1);
    }
    connect(size_group, &QButtonGroup::idClicked, this, [=,this](int id) {
        this->params.fontSize = sizes[id].second;
        emit paramsChanged(this->params);
    });

    // color: the aircraft color first
    auto color_lay = row("Color");
    auto colors = QList<QColor>{ac->getColor(), QColor("#111418"), QColor("#ffffff"), QColor("#2060ff"),
                                QColor("#d92d20"), QColor("#1a7f37"), QColor("#f2c46d"), QColor("#8250df")};
    auto color_group = new QButtonGroup(this);
    for(int i=0; i<colors.size(); i++) {
        auto b = new QPushButton(this);
        b->setCheckable(true);
        b->setChecked(colors[i] == params.color);
        b->setFixedSize(24, 24);
        b->setToolTip(i == 0 ? "Aircraft color" : colors[i].name());
        b->setStyleSheet(QString(
            "QPushButton {background: %1; border: 1px solid #8c959f; border-radius: 12px;}"
            "QPushButton:checked {border: 3px solid #1d5fc9;}").arg(colors[i].name()));
        color_group->addButton(b, i);
        color_lay->addWidget(b);
    }
    color_lay->addStretch(1);
    connect(color_group, &QButtonGroup::idClicked, this, [=,this](int id) {
        this->params.color = colors[id];
        emit paramsChanged(this->params);
    });

    if(!qApp->styleSheet().contains("papget_size")) {
        setStyleSheet(styleSheet() +
            "QPushButton#papget_size {border: 1px solid #c4ccd5; border-radius: 6px; background: #ffffff;}"
            "QPushButton#papget_size:checked {background: #dde7f7; color: #123f88; font-weight: bold;}");
    }

    auto remove = addButton("Remove", DANGER_OUTLINE, 0);
    auto cancel = addButton("Cancel");
    auto apply = addButton("Apply", PRIMARY);
    connect(remove, &QPushButton::clicked, this, [=,this]() {
        emit removeRequested();
        accept();
    });
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(apply, &QPushButton::clicked, this, [=,this]() {
        emit paramsChanged(this->params);
        accept();
    });
}
