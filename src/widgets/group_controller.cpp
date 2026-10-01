#include "group_controller.h"
#include "AircraftManager.h"
#include "dispatcher_ui.h"
#include "gcs_utils.h"
#include "pprz_dispatcher.h"
#include "double_slider.h"
#include "switch.h"

#include <QComboBox>
#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QSet>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTabWidget>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
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

static void logActionSkipped(const QString &groupName, const QString &actionName,
                             const QString &aircraftName, const QString &reason)
{
    qWarning().noquote() << QString("Group '%1', action '%2': skipped drone '%3': %4")
                            .arg(groupName, actionName, aircraftName, reason);
}

static QList<QPair<Aircraft *, Setting *>> actionTargets(
    const QStringList &members, const QString &variable,
    const std::function<void(const QString &, const QString &)> &onSkipped = {})
{
    QList<QPair<Aircraft *, Setting *>> targets;
    QSet<QString> foundAircraft;
    for (const QString &aircraftName : members) {
        if (foundAircraft.contains(aircraftName)) continue;
        foundAircraft.insert(aircraftName);
        const auto aircraft = AircraftManager::get()->getAircraftByName(aircraftName);
        if (!aircraft.has_value()) {
            if (onSkipped) onSkipped(aircraftName, QStringLiteral("aircraft is not loaded"));
            continue;
        }
        if (!aircraft.value()->isReal()) {
            if (onSkipped) onSkipped(aircraftName, QStringLiteral("aircraft is simulated"));
            continue;
        }

        bool foundSetting = false;
        for (Setting *setting : aircraft.value()->getSettingMenu()->getAllSettings()) {
            if (setting->getFullName() == variable) {
                targets.append(qMakePair(aircraft.value(), setting));
                foundSetting = true;
                break;
            }
        }
        if (!foundSetting && onSkipped) {
            onSkipped(aircraftName, QString("setting '%1' is not available").arg(variable));
        }
    }
    return targets;
}

static int actionDecimalPlaces(double value)
{
    if (!std::isfinite(value)) return 0;
    double scale = 1;
    for (int places = 0; places <= 9; ++places, scale *= 10) {
        const double scaled = value * scale;
        if (std::abs(scaled - std::round(scaled)) < 1e-5) return places;
    }
    return 9;
}

static QString actionNumber(double value, int precision)
{
    QString result = QString::number(value, 'f', precision);
    if (result.contains('.')) {
        while (result.endsWith('0')) result.chop(1);
        if (result.endsWith('.')) result.chop(1);
    }
    return result == "-0" ? QStringLiteral("0") : result;
}

GroupController::GroupController(QWidget *parent): QWidget(parent)
{
    if (appConfig()->contains("SWARM_CONF_PATH")) {
        configPath = appConfig()->value("SWARM_CONF_PATH").toString();
        if (configPath.trimmed().isEmpty()) {
            throw std::runtime_error("--swarm-conf needs a configuration file path");
        }
    } else {
        auto settings = getAppSettings();
        const QString configuredFile = settings.value("groups_conf").toString().trimmed();
        if (configuredFile.isEmpty()) {
            throw std::runtime_error(
                "No swarm configuration file is configured. Set 'groups_conf' in app settings "
                "or pass --swarm-conf <file>.");
        }
        configPath = user_or_app_path(configuredFile);
    }

    QFile file(configPath);
    if (!file.exists()) {
        throw std::runtime_error(
            QString("Swarm configuration file '%1' does not exist").arg(configPath).toStdString());
    }
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error(QString("Could not open swarm configuration file '%1': %2")
                                 .arg(configPath, file.errorString()).toStdString());
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
            GroupAction groupAction;
            groupAction.variable = action.attribute("var");
            groupAction.block = action.attribute("block");
            groupAction.hasValue = action.hasAttribute("value");
            groupAction.value = action.attribute("value");
            if (groupAction.variable.isEmpty() == groupAction.block.isEmpty() ||
                (groupAction.hasValue && (groupAction.value.isEmpty() ||
                                          !groupAction.block.isEmpty()))) continue;
            groupAction.title = action.attribute("name", groupAction.block.isEmpty()
                                               ? groupAction.variable : groupAction.block);
            groupActions[groupName].append(groupAction);
        }

        for (QDomElement status = group.firstChildElement("status"); !status.isNull();
             status = status.nextSiblingElement("status")) {
            StatusRule rule;
            const QString messageSource = status.attribute("msg");
            const QString settingSource = status.attribute("var");
            const QString blockSource = status.attribute("block");
            const int sourceCount = int(!messageSource.isEmpty())
                                    + int(!settingSource.isEmpty()) + int(!blockSource.isEmpty());
            if (sourceCount != 1 || (blockSource.isEmpty() && !status.hasAttribute("value"))) {
                throw std::runtime_error("Each status needs one msg, var, or block; msg and var need a value");
            }
            rule.type = !blockSource.isEmpty() ? StatusSource::Block
                        : !settingSource.isEmpty() ? StatusSource::Setting : StatusSource::Message;
            rule.source = rule.type == StatusSource::Block ? blockSource
                          : rule.type == StatusSource::Setting ? settingSource : messageSource;
            rule.title = status.attribute("name", rule.source);
            if (rule.title.isEmpty()) rule.title = rule.source;
            if (rule.type == StatusSource::Block) {
                if (status.hasAttribute("value") || status.hasAttribute("operator")) {
                    throw std::runtime_error("Block status uses only the block name and optional display name");
                }
                messageFields["NAV_STATUS"].insert("cur_block");
            } else {
                rule.expected = status.attribute("value");
                rule.comparison = status.attribute("operator", "eq");
                if (!QStringList{"eq", "ne", "lt", "le", "gt", "ge"}.contains(rule.comparison)) {
                    throw std::runtime_error("Unknown status comparison operator");
                }
            }
            if (rule.type == StatusSource::Message) {
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
    saveNotice = new QLabel(groupsPage);
    saveNotice->setWordWrap(true);
    saveNotice->setStyleSheet(QStringLiteral("QLabel { color: #ef6c00; }"));
    saveNotice->hide();
    pageLayout->addWidget(saveNotice);
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
        auto noActiveWarning = new QLabel(tr("No active drones in this group"), groupPage);
        noActiveWarning->setWordWrap(true);
        noActiveWarning->setStyleSheet(QStringLiteral("QLabel { color: #ef6c00; }"));
        noActiveWarning->hide();
        groupLayout->addWidget(noActiveWarning);
        noActiveWarningLabels.insert(groupName, noActiveWarning);
        for (StatusRule &rule : statusRules[groupName]) {
            rule.label = new QLabel(rule.title, groupPage);
            rule.label->setAlignment(Qt::AlignCenter);
            rule.label->setMinimumHeight(28);
            groupLayout->addWidget(rule.label);
        }
        auto blockWarning = new QLabel(groupPage);
        blockWarning->setWordWrap(true);
        blockWarning->setStyleSheet(QStringLiteral("QLabel { color: #ef6c00; }"));
        blockWarning->hide();
        groupLayout->addWidget(blockWarning);
        blockWarningLabels.insert(groupName, blockWarning);
        if (!groupActions.value(groupName).isEmpty()) {
            auto actionsBox = new QGroupBox(tr("Actions"), groupPage);
            actionsBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
            auto actionsLayout = new QVBoxLayout(actionsBox);
            int actionIndex = 0;
            for (const GroupAction &action : groupActions.value(groupName)) {
                if (actionIndex++ > 0) {
                    auto line = new QFrame(actionsBox);
                    line->setFrameShape(QFrame::HLine);
                    line->setFrameShadow(QFrame::Sunken);
                    actionsLayout->addWidget(line);
                }
                if (!action.block.isEmpty() || action.hasValue) {
                    auto button = new QPushButton(action.title, actionsBox);
                    actionsLayout->addWidget(button);
                    connect(button, &QPushButton::clicked, this,
                            [this, groupName, action]() {
                                if (!action.block.isEmpty()) {
                                    runBlockAction(groupName, action);
                                } else {
                                    runSettingAction(groupName, action, action.value);
                                }
                            });
                    continue;
                }

                ActionControl control;
                control.groupName = groupName;
                control.action = action;
                auto editor = new QWidget(actionsBox);
                editor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                auto editorLayout = new QVBoxLayout(editor);
                editorLayout->setContentsMargins(0, 0, 0, 0);
                auto titleRow = new QHBoxLayout;
                titleRow->addWidget(new QLabel(action.title, editor));
                titleRow->addStretch();
                control.currentValue = new QPushButton(QStringLiteral("---"), editor);
                control.currentValue->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                control.currentValue->setToolTip(tr("Request current value from group drones"));
                titleRow->addWidget(control.currentValue);
                control.commit = new QToolButton(editor);
                control.commit->setText(QStringLiteral("✓"));
                control.commit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                control.commit->setToolTip(tr("Commit"));
                control.commit->setEnabled(false);
                titleRow->addWidget(control.commit);
                editorLayout->addLayout(titleRow);
                control.selector = new QStackedWidget(editor);
                control.selector->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                editorLayout->addWidget(control.selector);

                control.unavailable = new QLabel(tr("Waiting for a group drone with this setting"), control.selector);
                control.selector->addWidget(control.unavailable);
                control.choices = new QComboBox(control.selector);
                control.selector->addWidget(control.choices);

                auto togglePage = new QWidget(control.selector);
                auto toggleLayout = new QHBoxLayout(togglePage);
                toggleLayout->setContentsMargins(0, 0, 0, 0);
                control.offLabel = new QLabel(togglePage);
                control.toggle = new Switch(togglePage);
                control.onLabel = new QLabel(togglePage);
                toggleLayout->addWidget(control.offLabel);
                toggleLayout->addWidget(control.toggle);
                toggleLayout->addWidget(control.onLabel);
                toggleLayout->addStretch();
                control.selector->addWidget(togglePage);

                auto sliderPage = new QWidget(control.selector);
                auto sliderLayout = new QHBoxLayout(sliderPage);
                sliderLayout->setContentsMargins(0, 0, 0, 0);
                control.slider = new DoubleSlider(Qt::Horizontal, sliderPage);
                control.sliderValue = new QLabel(sliderPage);
                control.unitLabel = new QLabel(sliderPage);
                sliderLayout->addWidget(control.slider, 1);
                sliderLayout->addWidget(control.sliderValue);
                sliderLayout->addWidget(control.unitLabel);
                control.selector->addWidget(sliderPage);
                connect(control.slider, &DoubleSlider::doubleValueChanged,
                        control.sliderValue, [this, controlIndex = actionControls.size()](double value) {
                            const ActionControl &selected = actionControls.at(controlIndex);
                            selected.sliderValue->setText(actionNumber(
                                std::clamp(value, selected.sliderMinimum, selected.sliderMaximum),
                                selected.sliderPrecision));
                        });

                actionsLayout->addWidget(editor);
                const int controlIndex = actionControls.size();
                actionControls.append(control);
                connect(control.currentValue, &QPushButton::clicked, this, [this, controlIndex]() {
                    ActionControl &selected = actionControls[controlIndex];
                    const auto targets = actionTargets(memberships.value(selected.groupName),
                        selected.action.variable,
                        [&selected](const QString &aircraftName, const QString &reason) {
                            logActionSkipped(selected.groupName,
                                QString("Request current value: %1").arg(selected.action.title),
                                aircraftName, reason);
                        });
                    selected.pendingValues.clear();
                    for (const auto &[aircraft, setting] : targets) {
                        (void)setting;
                        selected.pendingValues.insert(aircraft->getId());
                    }
                    refreshActionValues();
                    for (const auto &[aircraft, setting] : targets) {
                        pprzlink::Message request(
                            PprzDispatcher::get()->getDict()->getDefinition("GET_DL_SETTING"));
                        request.addField("ac_id", aircraft->getId());
                        request.addField("index", setting->getNo());
                        PprzDispatcher::get()->sendMessage(request);
                    }
                });
                connect(control.commit, &QToolButton::clicked, this, [this, controlIndex]() {
                    const ActionControl &selected = actionControls.at(controlIndex);
                    const QString value = actionInputValue(selected);
                    if (!value.isEmpty()) {
                        runSettingAction(selected.groupName, selected.action, value);
                    }
                });
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
            this, [this](const QString &aircraftId, Setting *setting, float) {
                for (ActionControl &control : actionControls) {
                    if (control.action.variable == setting->getFullName()) {
                        control.pendingValues.remove(aircraftId);
                    }
                }
                refreshStatuses();
                refreshActionValues();
            });
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
    refreshActionControls();
}

void GroupController::refreshStatuses()
{
    const auto liveness = currentLiveness();
    for (const QString &groupName : groupNames) {
        const QStringList aircraftNames = memberships.value(groupName);
        const bool hasActiveDrone = std::any_of(aircraftNames.cbegin(), aircraftNames.cend(),
            [&liveness](const QString &aircraftName) {
                return liveness.value(aircraftName, Liveness::Offline) == Liveness::Active;
            });
        noActiveWarningLabels.value(groupName)->setVisible(!hasActiveDrone);
        QSet<QString> blockWarnings;
        for (const StatusRule &rule : statusRules.value(groupName)) {
            QSet<QString> failingAircraft;
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
                    if (rule.type == StatusSource::Setting) {
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
                    } else if (rule.type == StatusSource::Message) {
                        const auto values = messageValues.value(aircraftName);
                        const auto found = values.constFind(rule.source);
                        if (found != values.cend()) {
                            actual = found.value();
                            enumValues = messageEnums.value(rule.source);
                            hasValue = true;
                        }
                    } else {
                        const auto &blocks = aircraft.value()->getFlightPlan()->getBlocks();
                        const auto block = std::find_if(blocks.cbegin(), blocks.cend(),
                            [&rule](const auto &candidate) {
                                return candidate->getName() == rule.source;
                            });
                        if (block == blocks.cend()) {
                            blockWarnings.insert(tr("%1: block '%2' is missing for %3")
                                                 .arg(rule.title, rule.source, aircraftName));
                            const QString warningKey = groupName + QChar(0x1f) + aircraftName
                                                       + QChar(0x1f) + rule.source;
                            if (!missingStatusBlockWarnings.contains(warningKey)) {
                                missingStatusBlockWarnings.insert(warningKey);
                                qWarning().noquote() << QString(
                                    "Group '%1', status '%2': drone '%3' has no flight plan block '%4'")
                                    .arg(groupName, rule.title, aircraftName, rule.source);
                            }
                        } else {
                            const auto values = messageValues.value(aircraftName);
                            const auto found = values.constFind("NAV_STATUS.cur_block");
                            if (found != values.cend()) {
                                bool validIndex = false;
                                const uint currentBlock = found.value().toUInt(&validIndex);
                                if (validIndex) matches = currentBlock == (*block)->getNo();
                            }
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
        QLabel *warningLabel = blockWarningLabels.value(groupName);
        QStringList warnings = blockWarnings.values();
        std::sort(warnings.begin(), warnings.end());
        warningLabel->setText(warnings.join('\n'));
        warningLabel->setVisible(!warnings.isEmpty());
    }
}

QString GroupController::actionInputValue(const ActionControl &control) const
{
    switch (control.input) {
    case ActionInput::Choices:
        return control.choices->currentText();
    case ActionInput::Toggle:
        return control.toggleValues.value(control.toggle->isChecked() ? 1 : 0);
    case ActionInput::Slider:
        return actionNumber(std::clamp(control.slider->doubleValue(),
                                       control.sliderMinimum, control.sliderMaximum),
                            control.sliderPrecision);
    case ActionInput::Unavailable:
        return {};
    }
    return {};
}

void GroupController::refreshActionControls()
{
    for (ActionControl &control : actionControls) {
        const QString previous = actionInputValue(control);
        const auto targets = actionTargets(memberships.value(control.groupName),
                                           control.action.variable);
        auto unavailable = [&](const QString &message) {
            control.unavailable->setText(message);
            control.input = ActionInput::Unavailable;
            control.selector->setCurrentIndex(0);
            control.commit->setEnabled(false);
        };
        if (targets.isEmpty()) {
            unavailable(tr("Waiting for a group drone with this setting"));
            continue;
        }

        const bool allEnumerated = std::all_of(targets.cbegin(), targets.cend(),
            [](const auto &target) { return !target.second->getValues().isEmpty(); });
        const bool anyEnumerated = std::any_of(targets.cbegin(), targets.cend(),
            [](const auto &target) { return !target.second->getValues().isEmpty(); });
        if (anyEnumerated && !allEnumerated) {
            unavailable(tr("These drones use incompatible controls for this setting"));
            continue;
        }
        if (allEnumerated) {
            QStringList commonValues;
            for (const QString &candidate : targets.first().second->getValues()) {
                if (std::all_of(targets.cbegin(), targets.cend(),
                    [&candidate](const auto &target) {
                        return target.second->getValues().contains(candidate);
                    })) {
                    commonValues.append(candidate);
                }
            }
            if (commonValues.isEmpty()) {
                unavailable(tr("No common values for these drones"));
                continue;
            }
            if (commonValues.size() == 2) {
                control.toggleValues = commonValues;
                control.offLabel->setText(commonValues.at(0));
                control.onLabel->setText(commonValues.at(1));
                control.toggle->setChecked(previous == commonValues.at(1));
                control.input = ActionInput::Toggle;
                control.selector->setCurrentIndex(2);
            } else {
                QSignalBlocker blocker(control.choices);
                control.choices->clear();
                control.choices->addItems(commonValues);
                if (commonValues.contains(previous)) {
                    control.choices->setCurrentText(previous);
                }
                control.input = ActionInput::Choices;
                control.selector->setCurrentIndex(1);
            }
        } else {
            double minimum = -std::numeric_limits<double>::infinity();
            double maximum = std::numeric_limits<double>::infinity();
            double step = 0;
            QString unit = targets.first().second->getDisplayUnit();
            bool sameUnit = true;
            for (const auto &target : targets) {
                const auto [low, high, increment] = target.second->getBounds();
                minimum = std::max(minimum, static_cast<double>(low));
                maximum = std::min(maximum, static_cast<double>(high));
                if (std::isfinite(increment) && increment > 0) {
                    step = std::max(step, static_cast<double>(increment));
                }
                if (target.second->getDisplayUnit() != unit) sameUnit = false;
            }
            if (!sameUnit) {
                unavailable(tr("These drones use different units for this setting"));
                continue;
            }
            if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum > maximum) {
                unavailable(tr("No common range for these drones"));
                continue;
            }
            if (minimum < maximum && step >= maximum - minimum) {
                control.toggleValues = {actionNumber(minimum, actionDecimalPlaces(minimum)),
                                        actionNumber(maximum, actionDecimalPlaces(maximum))};
                const QString suffix = unit.isEmpty() ? QString() : " " + unit;
                control.offLabel->setText(control.toggleValues.at(0) + suffix);
                control.onLabel->setText(control.toggleValues.at(1) + suffix);
                control.toggle->setChecked(previous == control.toggleValues.at(1));
                control.input = ActionInput::Toggle;
                control.selector->setCurrentIndex(2);
            } else {
                if (step <= 0) step = 1;
                if (maximum > minimum) {
                    step = std::max(step, (maximum - minimum) / 10000);
                }
                control.sliderMinimum = minimum;
                control.sliderMaximum = maximum;
                control.sliderPrecision = std::max({actionDecimalPlaces(minimum),
                                                    actionDecimalPlaces(maximum),
                                                    actionDecimalPlaces(step)});
                control.slider->setDoubleRange(minimum, maximum, step);
                bool previousIsNumber = false;
                double selected = previous.toDouble(&previousIsNumber);
                if (!previousIsNumber) {
                    Setting *first = targets.first().second;
                    selected = first->getValue().value_or(0.f) * first->getSendCoef();
                }
                control.slider->setDoubleValue(std::clamp(selected, minimum, maximum));
                control.sliderValue->setText(actionNumber(
                    std::clamp(control.slider->doubleValue(), minimum, maximum),
                    control.sliderPrecision));
                control.unitLabel->setText(unit);
                control.input = ActionInput::Slider;
                control.selector->setCurrentIndex(3);
            }
        }
        control.commit->setEnabled(true);
    }
    refreshActionValues();
}

void GroupController::refreshActionValues()
{
    for (ActionControl &control : actionControls) {
        const auto targets = actionTargets(memberships.value(control.groupName),
                                           control.action.variable);
        QSet<QString> targetIds;
        for (const auto &[aircraft, setting] : targets) {
            (void)setting;
            targetIds.insert(aircraft->getId());
        }
        control.pendingValues.intersect(targetIds);
        control.currentValue->setEnabled(!targets.isEmpty());
        QString display = QStringLiteral("---");
        if (!control.pendingValues.isEmpty()) {
            control.currentValue->setText(QStringLiteral("?"));
            continue;
        }
        if (targets.isEmpty()) {
            control.currentValue->setText(display);
            continue;
        }

        const bool enumerated = !targets.first().second->getValues().isEmpty();
        const QString unit = targets.first().second->getDisplayUnit();
        QString sharedChoice;
        double sharedNumber = 0;
        double sharedStep = 0;
        int precision = 0;
        bool allSimilar = true;
        bool first = true;
        for (const auto &[aircraft, setting] : targets) {
            (void)aircraft;
            const auto value = setting->getValue();
            if (!value.has_value() || !std::isfinite(value.value()) ||
                enumerated != !setting->getValues().isEmpty()) {
                allSimilar = false;
                break;
            }
            if (enumerated) {
                const double raw = value.value();
                const int index = static_cast<int>(std::round(raw));
                if (raw < 0 || raw >= setting->getValues().size() ||
                    std::abs(raw - index) > 1e-3) {
                    allSimilar = false;
                    break;
                }
                const QString choice = setting->getValues().at(index);
                if (!first && choice != sharedChoice) {
                    allSimilar = false;
                    break;
                }
                sharedChoice = choice;
            } else {
                if (setting->getDisplayUnit() != unit) {
                    allSimilar = false;
                    break;
                }
                const double number = static_cast<double>(value.value()) * setting->getSendCoef();
                if (!std::isfinite(number)) {
                    allSimilar = false;
                    break;
                }
                const auto [minimum, maximum, step] = setting->getBounds();
                (void)minimum;
                (void)maximum;
                precision = std::max({precision, actionDecimalPlaces(number),
                                      actionDecimalPlaces(step)});
                const double tolerance = std::max({1e-5, std::abs(number) * 1e-6,
                    std::abs(sharedNumber) * 1e-6,
                    std::max(std::abs(static_cast<double>(step)), sharedStep) * 1e-3});
                if (!first && std::abs(number - sharedNumber) > tolerance) {
                    allSimilar = false;
                    break;
                }
                if (first) sharedNumber = number;
                sharedStep = std::max(sharedStep, std::abs(static_cast<double>(step)));
            }
            first = false;
        }
        if (allSimilar && !first) {
            display = enumerated ? sharedChoice : actionNumber(sharedNumber, precision);
        }
        control.currentValue->setText(display);
    }
}

void GroupController::runSettingAction(const QString &groupName, const GroupAction &action,
                                       const QString &value)
{
    const auto targets = actionTargets(memberships.value(groupName), action.variable,
        [&groupName, &action](const QString &aircraftName, const QString &reason) {
            logActionSkipped(groupName, action.title, aircraftName, reason);
        });
    for (const auto &[aircraft, setting] : targets) {
        const int enumIndex = setting->getValues().indexOf(value);
        float rawValue;
        if (enumIndex >= 0) {
            rawValue = static_cast<float>(enumIndex);
        } else {
            bool numeric = false;
            const double displayedValue = value.toDouble(&numeric);
            if (!numeric) {
                logActionSkipped(groupName, action.title, aircraft->name(),
                    QString("value '%1' is not a valid choice or number for setting '%2'")
                        .arg(value, action.variable));
                continue;
            }
            rawValue = static_cast<float>(displayedValue / setting->getSendCoef());
        }
        if (std::isfinite(rawValue)) {
            aircraft->setSetting(setting, rawValue);
            setting->setUserValue(rawValue);
        } else {
            logActionSkipped(groupName, action.title, aircraft->name(),
                QString("value '%1' cannot be converted for setting '%2'")
                    .arg(value, action.variable));
        }
    }
}

void GroupController::runBlockAction(const QString &groupName, const GroupAction &action)
{
    QSet<QString> foundAircraft;
    for (const QString &aircraftName : memberships.value(groupName)) {
        if (foundAircraft.contains(aircraftName)) continue;
        foundAircraft.insert(aircraftName);
        const auto aircraft = AircraftManager::get()->getAircraftByName(aircraftName);
        if (!aircraft.has_value()) {
            logActionSkipped(groupName, action.title, aircraftName,
                             QStringLiteral("aircraft is not loaded"));
            continue;
        }
        if (!aircraft.value()->isReal()) {
            logActionSkipped(groupName, action.title, aircraftName,
                             QStringLiteral("aircraft is simulated"));
            continue;
        }

        bool foundBlock = false;
        for (const auto &block : aircraft.value()->getFlightPlan()->getBlocks()) {
            if (block->getName() != action.block) continue;
            pprzlink::Message message(PprzDispatcher::get()->getDict()->getDefinition("JUMP_TO_BLOCK"));
            message.addField("ac_id", aircraft.value()->getId());
            message.addField("block_id", block->getNo());
            PprzDispatcher::get()->sendMessage(message);
            foundBlock = true;
            break;
        }
        if (!foundBlock) {
            logActionSkipped(groupName, action.title, aircraftName,
                             QString("flight plan block '%1' is not available").arg(action.block));
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
    QString error;
    if (saveMembership(error)) {
        saveNotice->hide();
    } else {
        saveNotice->setText(tr("Group changes are active for this session but could not be saved to %1: %2")
                            .arg(configPath, error));
        saveNotice->show();
        qWarning().noquote() << saveNotice->text();
    }
    refreshStatuses();
    refreshActionControls();
}

bool GroupController::saveMembership(QString &error)
{
    if (!QFileInfo(configPath).isWritable()) {
        error = tr("Configuration file is not writable");
        return false;
    }
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
    for (const QString &groupName : groupNames) {
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

        const QStringList desiredNames = memberships.value(groupName);
        QSet<QString> existingNames;
        for (QDomElement ac = group.firstChildElement("ac"); !ac.isNull();) {
            QDomElement next = ac.nextSiblingElement("ac");
            const QString name = ac.attribute("name");
            if (!desiredNames.contains(name) || existingNames.contains(name)) {
                group.removeChild(ac);
            } else {
                existingNames.insert(name);
            }
            ac = next;
        }
        for (const QString &name : desiredNames) {
            if (existingNames.contains(name)) continue;
            QDomElement ac = document.createElement("ac");
            ac.setAttribute("name", name);
            QDomElement next = group.firstChildElement("action");
            if (next.isNull()) next = group.firstChildElement("status");
            if (next.isNull()) {
                group.appendChild(ac);
            } else {
                group.insertBefore(ac, next);
            }
            existingNames.insert(name);
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
