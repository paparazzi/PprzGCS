#include "group_controller.h"
#include "AircraftManager.h"
#include "dispatcher_ui.h"
#include "gcs_utils.h"
#include "pprz_dispatcher.h"

#include <QDomDocument>
#include <QFile>
#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QGroupBox>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QSet>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <stdexcept>

static bool statusMatches(QString actual, QString expected, const QString &comparison,
                          const QStringList &enumValues)
{
    const int actualIndex = enumValues.indexOf(actual);
    const int expectedIndex = enumValues.indexOf(expected);
    if (actualIndex >= 0) actual = QString::number(actualIndex);
    if (expectedIndex >= 0) expected = QString::number(expectedIndex);

    bool actualIsNumber = false;
    bool expectedIsNumber = false;
    const double actualNumber = actual.toDouble(&actualIsNumber);
    const double expectedNumber = expected.toDouble(&expectedIsNumber);
    if (comparison == "eq") {
        return actualIsNumber && expectedIsNumber ? actualNumber == expectedNumber
                                                 : actual == expected;
    }
    if (comparison == "ne") {
        return actualIsNumber && expectedIsNumber ? actualNumber != expectedNumber
                                                 : actual != expected;
    }
    if (!actualIsNumber || !expectedIsNumber) return false;
    if (comparison == "lt") return actualNumber < expectedNumber;
    if (comparison == "le") return actualNumber <= expectedNumber;
    if (comparison == "gt") return actualNumber > expectedNumber;
    if (comparison == "ge") return actualNumber >= expectedNumber;
    return false;
}

GroupController::GroupController(QWidget *parent): QWidget(parent)
{
    auto settings = getAppSettings();
    configPath = user_or_app_path(settings.value("groups_conf").toString());

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error(file.errorString().toStdString());
    }
    QDomDocument document;
    if (!document.setContent(&file) || document.documentElement().tagName() != "swarm") {
        throw std::runtime_error("Groups config must contain a valid <swarm> XML element");
    }
    const QDomElement root = document.documentElement();
    for (QDomElement group = root.firstChildElement("group"); !group.isNull(); group = group.nextSiblingElement("group")) {
        const QString groupName = group.attribute("name");
        if (groupName.isEmpty() || memberships.contains(groupName)) {
            throw std::runtime_error("Group names must be present and unique");
        }
        groupNames.append(groupName);
        QStringList aircraftNames;
        for (QDomElement ac = group.firstChildElement("ac"); !ac.isNull(); ac = ac.nextSiblingElement("ac")) {
            aircraftNames.append(ac.attribute("name"));
        }
        memberships.insert(groupName, aircraftNames);

        for (QDomElement action = group.firstChildElement("action"); !action.isNull();
             action = action.nextSiblingElement("action")) {
            if (!action.hasAttribute("var") || !action.hasAttribute("value")) continue;
            SettingAction settingAction;
            settingAction.variable = action.attribute("var");
            settingAction.value = action.attribute("value");
            if (settingAction.variable.isEmpty() || settingAction.value.isEmpty()) continue;
            settingAction.title = action.attribute("name", settingAction.variable);
            settingActions[groupName].append(settingAction);
        }

        for (QDomElement status = group.firstChildElement("status"); !status.isNull();
             status = status.nextSiblingElement("status")) {
            StatusRule rule;
            const QString messageSource = status.attribute("msg");
            const QString settingSource = status.attribute("var");
            if (messageSource.isEmpty() == settingSource.isEmpty() || !status.hasAttribute("value")) {
                throw std::runtime_error("Each status needs one msg or var and a value");
            }
            rule.isSetting = !settingSource.isEmpty();
            rule.source = rule.isSetting ? settingSource : messageSource;
            rule.title = status.attribute("name", rule.source);
            if (rule.title.isEmpty()) rule.title = rule.source;
            rule.expected = status.attribute("value");
            rule.comparison = status.attribute("operator", "eq");
            if (!QStringList{"eq", "ne", "lt", "le", "gt", "ge"}.contains(rule.comparison)) {
                throw std::runtime_error("Unknown status comparison operator");
            }
            if (!rule.isSetting) {
                const int separator = messageSource.indexOf('.');
                if (separator <= 0 || separator == messageSource.size() - 1) {
                    throw std::runtime_error("Status msg must be MESSAGE_NAME.field_name");
                }
                rule.messageName = messageSource.left(separator);
                rule.fieldName = messageSource.mid(separator + 1);
                messageFields[rule.messageName].insert(rule.fieldName);
            }
            statusRules[groupName].append(rule);
        }
    }

    // The message dictionary exposes raw values; the XML supplies names for enum values.
    QFile messagesFile(appConfig()->value("MESSAGES").toString());
    QDomDocument messagesDocument;
    if (messagesFile.open(QIODevice::ReadOnly) && messagesDocument.setContent(&messagesFile)) {
        const QDomElement protocol = messagesDocument.documentElement();
        for (QDomElement messageClass = protocol.firstChildElement("msg_class");
             !messageClass.isNull(); messageClass = messageClass.nextSiblingElement("msg_class")) {
            for (QDomElement message = messageClass.firstChildElement("message");
                 !message.isNull(); message = message.nextSiblingElement("message")) {
                const QString messageName = message.attribute("name");
                if (!messageFields.contains(messageName)) continue;
                for (QDomElement field = message.firstChildElement("field"); !field.isNull();
                     field = field.nextSiblingElement("field")) {
                    if (field.hasAttribute("values")) {
                        messageEnums.insert(messageName + "." + field.attribute("name"),
                                            field.attribute("values").split('|'));
                    }
                }
            }
        }
    }

    auto layout = new QVBoxLayout(this);
    tabWidget = new QTabWidget(this);
    layout->addWidget(tabWidget);
    auto groupsPage = new QWidget(tabWidget);
    tabWidget->addTab(groupsPage, tr("Groupes"));
    auto pageLayout = new QVBoxLayout(groupsPage);
    table = new QTableWidget(groupsPage);
    pageLayout->addWidget(table);
    table->setColumnCount(groupNames.size() + 1);
    table->setHorizontalHeaderLabels(QStringList{tr("Aircraft")} + groupNames);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->hide();
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::AllEditTriggers);

    for (const QString &groupName : groupNames) {
        auto groupPage = new QWidget(tabWidget);
        tabWidget->addTab(groupPage, groupName);
        auto groupLayout = new QVBoxLayout(groupPage);
        for (StatusRule &rule : statusRules[groupName]) {
            rule.label = new QLabel(rule.title, groupPage);
            rule.label->setAlignment(Qt::AlignCenter);
            rule.label->setMinimumHeight(28);
            groupLayout->addWidget(rule.label);
        }
        if (!settingActions.value(groupName).isEmpty()) {
            auto actionsBox = new QGroupBox(tr("Actions"), groupPage);
            auto actionsLayout = new QVBoxLayout(actionsBox);
            for (const SettingAction &action : settingActions.value(groupName)) {
                auto button = new QPushButton(action.title, actionsBox);
                actionsLayout->addWidget(button);
                connect(button, &QPushButton::clicked, this,
                        [this, groupName, action]() { runSettingAction(groupName, action); });
            }
            groupLayout->addWidget(actionsBox);
        }
        groupLayout->addStretch();
    }

    for (auto it = messageFields.cbegin(); it != messageFields.cend(); ++it) {
        const QString messageName = it.key();
        const QSet<QString> fields = it.value();
        PprzDispatcher::get()->bind(messageName, this,
            [this, messageName, fields](QString sender, pprzlink::Message message) {
                QString aircraftId = sender;
                if (!AircraftManager::get()->aircraftExists(aircraftId) &&
                    message.getDefinition().hasFieldName("ac_id")) {
                    message.getField("ac_id", aircraftId);
                }
                if (!AircraftManager::get()->aircraftExists(aircraftId)) return;
                const QString aircraftName = AircraftManager::get()->getAircraft(aircraftId)->name();
                for (const QString &field : fields) {
                    try {
                        const QString value = message.getFieldAsStr(field);
                        if (value != "NOTSET") {
                            messageValues[aircraftName].insert(messageName + "." + field, value);
                        }
                    } catch (const std::exception &) {
                        // A missing field leaves the corresponding status failed.
                    }
                }
                refreshStatuses();
            });
    }

    connect(table, &QTableWidget::itemChanged, this, &GroupController::membershipChanged);
    connect(DispatcherUi::get(), &DispatcherUi::settingUpdated,
            this, [this](const QString &, Setting *, float) { refreshStatuses(); });
    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config,
            this, [this](const QString &) { refreshTable(); });
    // ac_deleted is emitted before AircraftManager removes the aircraft.
    connect(DispatcherUi::get(), &DispatcherUi::ac_deleted, this,
            [this](const QString &aircraftId) {
                if (AircraftManager::get()->aircraftExists(aircraftId)) {
                    messageValues.remove(AircraftManager::get()->getAircraft(aircraftId)->name());
                }
                currentLiveness();
                QTimer::singleShot(0, this, &GroupController::refreshTable);
            });
    auto livenessTimer = new QTimer(this);
    connect(livenessTimer, &QTimer::timeout, this, [this]() {
        if (currentLiveness() != displayedLiveness) {
            refreshTable();
        }
    });
    livenessTimer->start(500);
    refreshTable();
}

QMap<QString, GroupController::Liveness> GroupController::currentLiveness()
{
    QSet<QString> allNames;
    QMap<QString, Liveness> states;
    for (Aircraft *aircraft : AircraftManager::get()->getAircrafts()) {
        if (aircraft->isReal()) {
            const QString name = aircraft->name();
            allNames.insert(name);
            const auto telemetry = aircraft->getStatus()->getTelemetryMessages();
            if (!telemetry.isEmpty()) {
                seenInTelemetry.insert(name);
                bool active = false;
                for (const auto &message : telemetry) {
                    float timeSinceLastMessage = 0;
                    message.getField("time_since_last_msg", timeSinceLastMessage);
                    if (timeSinceLastMessage <= 5) {
                        active = true;
                        break;
                    }
                }
                states.insert(name, active ? Liveness::Active : Liveness::Lost);
            }
        }
    }
    for (const QString &name : seenInTelemetry) {
        allNames.insert(name);
    }
    for (const QStringList &groupAircraft : memberships) {
        for (const QString &name : groupAircraft) {
            allNames.insert(name);
        }
    }

    for (const QString &name : allNames) {
        if (!states.contains(name)) {
            states.insert(name, seenInTelemetry.contains(name) ? Liveness::Lost : Liveness::Offline);
        }
    }
    return states;
}

void GroupController::refreshTable()
{
    QSignalBlocker blocker(table);
    displayedLiveness = currentLiveness();
    QStringList active, lost, offline;
    for (auto it = displayedLiveness.cbegin(); it != displayedLiveness.cend(); ++it) {
        switch (it.value()) {
        case Liveness::Active: active.append(it.key()); break;
        case Liveness::Lost: lost.append(it.key()); break;
        case Liveness::Offline: offline.append(it.key()); break;
        }
    }
    auto nameLess = [](const QString &a, const QString &b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    };
    std::sort(active.begin(), active.end(), nameLess);
    std::sort(lost.begin(), lost.end(), nameLess);
    std::sort(offline.begin(), offline.end(), nameLess);
    active.append(lost);
    active.append(offline);

    table->setRowCount(active.size());
    for (int row = 0; row < active.size(); ++row) {
        const QString &name = active.at(row);
        const Liveness state = displayedLiveness.value(name);
        const QString status = state == Liveness::Active ? tr("Active")
                               : state == Liveness::Lost ? tr("Lost") : tr("Offline");
        auto nameItem = new QTableWidgetItem(name);
        nameItem->setData(Qt::UserRole, name);
        nameItem->setFlags(Qt::ItemIsEnabled);
        nameItem->setToolTip(status);
        QFont font = nameItem->font();
        font.setBold(state != Liveness::Offline);
        nameItem->setFont(font);
        if (state == Liveness::Lost) {
            nameItem->setForeground(Qt::red);
        } else if (state == Liveness::Offline) {
            nameItem->setForeground(Qt::gray);
        }
        table->setItem(row, 0, nameItem);

        for (int column = 0; column < groupNames.size(); ++column) {
            auto checkbox = new QTableWidgetItem;
            checkbox->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            checkbox->setCheckState(memberships.value(groupNames.at(column)).contains(name)
                                        ? Qt::Checked : Qt::Unchecked);
            checkbox->setTextAlignment(Qt::AlignCenter);
            if (state == Liveness::Offline) {
                checkbox->setForeground(Qt::gray);
            }
            table->setItem(row, column + 1, checkbox);
        }
    }
    refreshStatuses();
}

void GroupController::refreshStatuses()
{
    const auto liveness = currentLiveness();
    for (const QString &groupName : groupNames) {
        for (const StatusRule &rule : statusRules.value(groupName)) {
            QSet<QString> failingAircraft;
            const QStringList aircraftNames = memberships.value(groupName);
            int activeCount = 0;
            int passingCount = 0;
            for (const QString &aircraftName : aircraftNames) {
                if (liveness.value(aircraftName, Liveness::Offline) != Liveness::Active) {
                    continue;
                }
                ++activeCount;
                bool matches = false;
                const auto aircraft = AircraftManager::get()->getAircraftByName(aircraftName);
                if (aircraft.has_value() && aircraft.value()->isReal()) {
                    QString actual;
                    QStringList enumValues;
                    bool hasValue = false;
                    if (rule.isSetting) {
                        for (Setting *setting : aircraft.value()->getSettingMenu()->getAllSettings()) {
                            if (setting->getFullName() == rule.source) {
                                const auto value = setting->getValue();
                                if (value.has_value()) {
                                    actual = QString::number(value.value(), 'g', 9);
                                    enumValues = setting->getValues();
                                    hasValue = true;
                                }
                                break;
                            }
                        }
                    } else {
                        const auto values = messageValues.value(aircraftName);
                        const auto found = values.constFind(rule.source);
                        if (found != values.cend()) {
                            actual = found.value();
                            enumValues = messageEnums.value(rule.source);
                            hasValue = true;
                        }
                    }
                    if (hasValue) {
                        matches = statusMatches(actual, rule.expected, rule.comparison, enumValues);
                    }
                }
                if (matches) {
                    ++passingCount;
                } else {
                    failingAircraft.insert(aircraftName);
                }
            }

            const char *color = passingCount == 0 ? "#c62828"
                                : passingCount == activeCount ? "#2e7d32" : "#ef6c00";
            rule.label->setStyleSheet(QString(
                "QLabel { background-color: %1; color: white; padding: 4px; border-radius: 3px; }").arg(color));
            QStringList names = failingAircraft.values();
            std::sort(names.begin(), names.end());
            rule.label->setToolTip(activeCount == 0 ? tr("No active drones") : names.join(", "));
        }
    }
}

void GroupController::runSettingAction(const QString &groupName, const SettingAction &action)
{
    for (const QString &aircraftName : memberships.value(groupName)) {
        const auto aircraft = AircraftManager::get()->getAircraftByName(aircraftName);
        if (!aircraft.has_value() || !aircraft.value()->isReal()) continue;

        for (Setting *setting : aircraft.value()->getSettingMenu()->getAllSettings()) {
            if (setting->getFullName() != action.variable) continue;

            const int enumIndex = setting->getValues().indexOf(action.value);
            float rawValue;
            if (enumIndex >= 0) {
                rawValue = static_cast<float>(enumIndex);
            } else {
                bool numeric = false;
                const double displayedValue = action.value.toDouble(&numeric);
                if (!numeric) break;
                rawValue = static_cast<float>(displayedValue / setting->getSendCoef());
            }
            if (std::isfinite(rawValue)) {
                aircraft.value()->setSetting(setting, rawValue);
                setting->setUserValue(rawValue);
            }
            break;
        }
    }
}

void GroupController::membershipChanged(QTableWidgetItem *item)
{
    if (item->column() == 0) {
        return;
    }
    const QString groupName = groupNames.at(item->column() - 1);
    const QString name = table->item(item->row(), 0)->data(Qt::UserRole).toString();
    const bool member = item->checkState() == Qt::Checked;
    if (memberships.value(groupName).contains(name) == member) {
        return;
    }

    QString error;
    if (!saveMembership(groupName, name, member, error)) {
        QSignalBlocker blocker(table);
        item->setCheckState(member ? Qt::Unchecked : Qt::Checked);
        QMessageBox::warning(this, tr("Group membership"),
                             tr("Could not save group membership: %1").arg(error));
        return;
    }

    if (member) {
        memberships[groupName].append(name);
    } else {
        memberships[groupName].removeAll(name);
        bool stillConfigured = false;
        for (const QStringList &groupAircraft : memberships) {
            if (groupAircraft.contains(name)) {
                stillConfigured = true;
                break;
            }
        }
        if (!stillConfigured) {
            // Remove a never-seen row after its last configured membership is removed.
            QTimer::singleShot(0, this, &GroupController::refreshTable);
        }
    }
    refreshStatuses();
}

bool GroupController::saveMembership(const QString &groupName, const QString &aircraftName,
                                     bool member, QString &error)
{
    QFile input(configPath);
    if (!input.open(QIODevice::ReadOnly)) {
        error = input.errorString();
        return false;
    }
    QDomDocument document;
    if (!document.setContent(&input) || document.documentElement().tagName() != "swarm") {
        error = tr("Invalid swarm XML file");
        return false;
    }
    input.close();

    const QDomElement root = document.documentElement();
    QDomElement group;
    for (QDomElement candidate = root.firstChildElement("group"); !candidate.isNull();
         candidate = candidate.nextSiblingElement("group")) {
        if (candidate.attribute("name") == groupName) {
            group = candidate;
            break;
        }
    }
    if (group.isNull()) {
        error = tr("Group %1 was not found").arg(groupName);
        return false;
    }

    bool found = false;
    for (QDomElement ac = group.firstChildElement("ac"); !ac.isNull();) {
        QDomElement next = ac.nextSiblingElement("ac");
        if (ac.attribute("name") == aircraftName) {
            found = true;
            if (!member) {
                group.removeChild(ac);
            }
        }
        ac = next;
    }
    if (member && !found) {
        QDomElement ac = document.createElement("ac");
        ac.setAttribute("name", aircraftName);
        QDomElement next = group.firstChildElement("action");
        if (next.isNull()) {
            next = group.firstChildElement("status");
        }
        if (next.isNull()) {
            group.appendChild(ac);
        } else {
            group.insertBefore(ac, next);
        }
    }

    QSaveFile output(configPath);
    if (!output.open(QIODevice::WriteOnly)) {
        error = output.errorString();
        return false;
    }
    const QByteArray bytes = document.toByteArray(2);
    if (output.write(bytes) != bytes.size() || !output.commit()) {
        error = output.errorString();
        return false;
    }
    return true;
}
