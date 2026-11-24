#include "audio_mic_settings.h"
#include "audio_recorder_status.h"
#include <QAudioSource>
#include <QMediaDevices>

AudioMicSettings::AudioMicSettings(QWidget *parent) : QWidget(parent),
    media_devices(new QMediaDevices(this))
{
    auto v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(6);
    rows_lay = new QVBoxLayout();
    rows_lay->setSpacing(6);
    v->addLayout(rows_lay);

    auto add = new QPushButton("Add a microphone", this);
    add->setObjectName("card_button");
    connect(add, &QPushButton::clicked, this, [=,this]() {
        MicConfig c;
        auto d = QMediaDevices::defaultAudioInput();
        c.device_id = d.id();
        c.device_name = d.description();
        addRow(c);
    });
    auto h = new QHBoxLayout();
    h->addWidget(add);
    h->addStretch(1);
    v->addLayout(h);

    connect(media_devices, &QMediaDevices::audioInputsChanged, this, &AudioMicSettings::refreshDevices);
}

AudioMicSettings::~AudioMicSettings() {
    for(auto r: std::as_const(rows)) {
        closeTest(r);
        delete r;
    }
}

void AudioMicSettings::setConfigs(QList<MicConfig> list) {
    const auto old = rows;
    for(auto r: old) {
        removeRow(r);
    }
    for(auto &c: list) {
        addRow(c);
    }
}

QList<MicConfig> AudioMicSettings::configs() {
    QList<MicConfig> list;
    for(auto r: std::as_const(rows)) {
        MicConfig c;
        c.device_id = r->device->currentData(Qt::UserRole).toByteArray();
        c.device_name = r->device->currentData(Qt::UserRole + 1).toString();
        for(auto &l: r->labels->text().split(";")) {
            c.labels.append(l.trimmed());
        }
        while(!c.labels.isEmpty() && c.labels.last().isEmpty()) {
            c.labels.removeLast();
        }
        c.record = r->record->isChecked();
        list.append(c);
    }
    return list;
}

void AudioMicSettings::addRow(MicConfig c) {
    auto r = new Row();
    r->saved = c;
    r->box = new QWidget(this);
    auto g = new QGridLayout(r->box);
    g->setContentsMargins(0, 0, 0, 0);
    g->setHorizontalSpacing(6);
    g->setVerticalSpacing(2);

    r->device = new QComboBox(r->box);
    r->device->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    r->device->setMinimumContentsLength(24);
    r->labels = new QLineEdit(c.labels.join("; "), r->box);
    r->labels->setPlaceholderText("Label per channel, e.g. Pilot; Safety pilot");
    r->labels->setToolTip("Who is on each channel, separated by ';' (e.g. with a DJI Mic receiver in stereo: "
                          "the first transmitter on the left channel, the second on the right)");
    r->meters = new QWidget(r->box);
    auto ml = new QHBoxLayout(r->meters);
    ml->setContentsMargins(0, 0, 0, 0);
    ml->setSpacing(3);
    r->record = new QCheckBox("Record", r->box);
    r->record->setChecked(c.record);
    r->record->setToolTip("Recorded with the others (unticked: kept in this profile, not recorded)");
    auto remove = new QToolButton(r->box);
    remove->setText(QString::fromUtf8("×"));
    remove->setToolTip("Remove this microphone");
    remove->setAutoRaise(true);
    r->channels = new QLabel(r->box);
    r->channels->setObjectName("link_secondary");
    r->channels->hide();

    g->addWidget(r->device, 0, 0);
    g->addWidget(r->labels, 0, 1);
    g->addWidget(r->meters, 0, 2);
    g->addWidget(r->record, 0, 3);
    g->addWidget(remove, 0, 4);
    g->addWidget(r->channels, 1, 0, 1, 5);
    g->setColumnStretch(1, 1);
    rows_lay->addWidget(r->box);
    rows.append(r);

    fillDevices(r);
    connect(r->device, &QComboBox::currentIndexChanged, this, [=,this]() {
        if(isVisible()) {
            openTest(r);
        }
    });
    connect(remove, &QToolButton::clicked, this, [=,this]() {removeRow(r);});
    if(isVisible()) {
        openTest(r);
    }
}

void AudioMicSettings::removeRow(Row* r) {
    closeTest(r);
    rows.removeAll(r);
    r->box->deleteLater();
    delete r;
}

void AudioMicSettings::fillDevices(Row* r) {
    // the selected one: the current choice, else the saved one
    QByteArray id = r->device->count() > 0 ? r->device->currentData(Qt::UserRole).toByteArray() : r->saved.device_id;
    QString name = r->device->count() > 0 ? r->device->currentData(Qt::UserRole + 1).toString() : r->saved.device_name;
    QSignalBlocker block(r->device);
    r->device->clear();
    int selected = -1;
    for(auto &d: QMediaDevices::audioInputs()) {
        r->device->addItem(d.description());
        r->device->setItemData(r->device->count() - 1, d.id(), Qt::UserRole);
        r->device->setItemData(r->device->count() - 1, d.description(), Qt::UserRole + 1);
        if(d.id() == id || (selected < 0 && d.description() == name)) {
            selected = r->device->count() - 1;
        }
    }
    if(selected < 0 && !(id.isEmpty() && name.isEmpty())) {
        // kept, until it is plugged in
        r->device->addItem(QString("%1 (not connected)").arg(name.isEmpty() ? QString(id) : name));
        r->device->setItemData(r->device->count() - 1, id, Qt::UserRole);
        r->device->setItemData(r->device->count() - 1, name, Qt::UserRole + 1);
        selected = r->device->count() - 1;
    }
    r->device->setCurrentIndex(std::max(selected, 0));
}

void AudioMicSettings::refreshDevices() {
    for(auto r: std::as_const(rows)) {
        auto before = r->device->currentText();
        fillDevices(r);
        if(isVisible() && (r->device->currentText() != before || r->source == nullptr)) {
            openTest(r);
        }
    }
}

void AudioMicSettings::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    for(auto r: std::as_const(rows)) {
        if(r->source == nullptr) {
            openTest(r);
        }
    }
}

void AudioMicSettings::hideEvent(QHideEvent* e) {
    QWidget::hideEvent(e);
    // the devices open only while the tab shows (the dialog may stay after it is closed)
    for(auto r: std::as_const(rows)) {
        closeTest(r);
    }
}

void AudioMicSettings::closeTest(Row* r) {
    if(r->io != nullptr) {
        r->io->disconnect(this);
    }
    if(r->source != nullptr) {
        r->source->disconnect(this);
        r->source->stop();
        r->source->deleteLater();
        r->source = nullptr;
        r->io = nullptr;
    }
}

void AudioMicSettings::openTest(Row* r) {
    closeTest(r);
    for(auto b: std::as_const(r->bars)) {
        b->deleteLater();
    }
    r->bars.clear();

    MicConfig c;
    c.device_id = r->device->currentData(Qt::UserRole).toByteArray();
    c.device_name = r->device->currentData(Qt::UserRole + 1).toString();
    auto device = AudioRecorder::findDevice(c);
    if(device.isNull()) {
        r->channels->setText("Not connected: recorded when it is plugged in");
        r->channels->show();
        return;
    }
    r->format = AudioRecorder::captureFormat(device);
    int ch = r->format.channelCount();
    // (the channels in the tooltip: a line under the row only for a problem)
    r->channels->hide();
    r->meters->setToolTip(ch == 1 ? QString("1 channel") :
                          ch == 2 ? QString("2 channels: left; right") : QString("%1 channels").arg(ch));
    for(int i = 0; i < ch; i++) {
        auto b = new AudioLevelBar(r->meters);
        r->meters->layout()->addWidget(b);
        r->bars.append(b);
    }

    // the level right now, to test it (falls back slowly, as a meter)
    r->source = new QAudioSource(device, r->format, this);
    r->io = r->source->start();
    if(r->io == nullptr) {
        r->channels->setText("Cannot open this device");
        r->channels->show();
        closeTest(r);
        return;
    }
    auto shown = std::make_shared<QList<float>>(ch, -120.f);
    connect(r->io, &QIODevice::readyRead, this, [=,this]() {
        if(r->io == nullptr) {
            return;
        }
        auto data = r->io->readAll();
        qint64 frames = data.size() / r->format.bytesPerFrame();
        if(frames == 0) {
            return;
        }
        auto p = AudioRecorder::peaks(data.constData(), frames, r->format);
        for(int i = 0; i < p.size() && i < r->bars.size(); i++) {
            (*shown)[i] = std::max(p[i], (*shown)[i] - 1.5f);
            r->bars[i]->setLevel((*shown)[i], false);
        }
    });
}
