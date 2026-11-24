#include "camera_settings.h"

CameraSettings::CameraSettings(QWidget *parent) : QWidget(parent)
{
    auto v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(6);
    rows_lay = new QVBoxLayout();
    rows_lay->setSpacing(6);
    v->addLayout(rows_lay);

    auto add = new QPushButton("Add a camera", this);
    add->setObjectName("card_button");
    connect(add, &QPushButton::clicked, this, [=,this]() {
        CameraConfig c;
        auto devs = VideoRecorder::devices();
        if(!devs.isEmpty()) {
            c.source = devs.first().path;
            c.name = devs.first().name;
        }
        c.label = QString("Camera %1").arg(rows.size() + 1);
        addRow(c);
    });
    auto h = new QHBoxLayout();
    h->addWidget(add);
    h->addStretch(1);
    v->addLayout(h);
}

void CameraSettings::setConfigs(QList<CameraConfig> list) {
    for(auto r: std::as_const(rows)) {
        r->box->deleteLater();
        delete r;
    }
    rows.clear();
    for(auto &c: list) {
        addRow(c);
    }
}

QList<CameraConfig> CameraSettings::configs() {
    QList<CameraConfig> list;
    for(auto r: std::as_const(rows)) {
        CameraConfig c;
        // a USB camera of the list, else the text: the URL of a stream
        int i = r->source->currentIndex();
        if(i >= 0 && r->source->itemText(i) == r->source->currentText() && r->source->itemData(i).isValid()) {
            c.source = r->source->itemData(i).toString();
            c.name = r->source->itemData(i, Qt::UserRole + 1).toString();
        } else {
            c.source = r->source->currentText().trimmed();
        }
        c.label = r->label->text().trimmed();
        c.record = r->record->isChecked();
        if(!c.source.isEmpty()) {
            list.append(c);
        }
    }
    return list;
}

void CameraSettings::fillSources(Row* r, CameraConfig c) {
    r->source->clear();
    int selected = -1;
    auto devs = VideoRecorder::devices();
    for(auto &d: devs) {
        r->source->addItem(QString("%1 (%2)").arg(d.name, d.path));
        r->source->setItemData(r->source->count() - 1, d.path);
        r->source->setItemData(r->source->count() - 1, d.name, Qt::UserRole + 1);
        if(c.source == d.path && (c.name.isEmpty() || c.name == d.name)) {
            selected = r->source->count() - 1;
        }
    }
    if(selected < 0 && c.source.startsWith("/dev/")) {
        for(int i = 0; i < r->source->count(); i++) {
            if(r->source->itemData(i, Qt::UserRole + 1).toString() == c.name) {
                selected = i;
            }
        }
        if(selected < 0) {
            // kept, until it is plugged in
            r->source->addItem(QString("%1 (%2, not connected)").arg(c.name, c.source));
            r->source->setItemData(r->source->count() - 1, c.source);
            r->source->setItemData(r->source->count() - 1, c.name, Qt::UserRole + 1);
            selected = r->source->count() - 1;
        }
    }
    if(selected >= 0) {
        r->source->setCurrentIndex(selected);
    } else {
        r->source->setCurrentIndex(-1);
        r->source->setEditText(c.source);   // a stream
    }
}

void CameraSettings::addRow(CameraConfig c) {
    auto r = new Row();
    r->box = new QWidget(this);
    auto g = new QHBoxLayout(r->box);
    g->setContentsMargins(0, 0, 0, 0);
    g->setSpacing(6);
    r->source = new QComboBox(r->box);
    r->source->setEditable(true);
    r->source->setInsertPolicy(QComboBox::NoInsert);
    r->source->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    r->source->setMinimumContentsLength(24);
    r->source->setToolTip("A USB camera of this computer, or the URL of a stream (e.g. rtsp://192.168.1.10:8554/video, "
                          "udp://:5000, srt://...)");
    r->source->lineEdit()->setPlaceholderText("USB camera, or rtsp://...");
    r->label = new QLineEdit(c.label, r->box);
    r->label->setPlaceholderText("Label, e.g. Pilot view");
    r->record = new QCheckBox("Record", r->box);
    r->record->setChecked(c.record);
    r->record->setToolTip("Recorded with the microphones (unticked: only shown in the Video panel)");
    auto remove = new QToolButton(r->box);
    remove->setText(QString::fromUtf8("×"));
    remove->setToolTip("Remove this camera");
    remove->setAutoRaise(true);
    g->addWidget(r->source, 1);
    g->addWidget(r->label, 1);
    g->addWidget(r->record);
    g->addWidget(remove);
    rows_lay->addWidget(r->box);
    rows.append(r);
    fillSources(r, c);
    connect(remove, &QToolButton::clicked, this, [=,this]() {
        rows.removeAll(r);
        r->box->deleteLater();
        delete r;
    });
}
