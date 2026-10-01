#ifndef GROUP_CONTROLLER_H
#define GROUP_CONTROLLER_H

#include <QMap>
#include <QSet>
#include <QStringList>
#include <QWidget>

class QTableWidget;
class QTableWidgetItem;
class QTabWidget;
class QLabel;
class QComboBox;
class QStackedWidget;
class QPushButton;
class QToolButton;
class DoubleSlider;
class Switch;

class GroupController : public QWidget
{
    Q_OBJECT

public:
    explicit GroupController(QWidget *parent = nullptr);

private:
    enum class Liveness { Active, Lost, Offline };
    enum class StatusSource { Message, Setting, Block };
    struct StatusRule {
        QString title;
        QString source;
        QString expected;
        QString comparison;
        QString messageName;
        QString fieldName;
        StatusSource type = StatusSource::Message;
        QLabel *label = nullptr;
    };
    struct GroupAction {
        QString title;
        QString variable;
        QString block;
        QString value;
        bool hasValue = false;
    };
    enum class ActionInput { Unavailable, Choices, Toggle, Slider };
    struct ActionControl {
        QString groupName;
        GroupAction action;
        QStackedWidget *selector = nullptr;
        QLabel *unavailable = nullptr;
        QComboBox *choices = nullptr;
        Switch *toggle = nullptr;
        QLabel *offLabel = nullptr;
        QLabel *onLabel = nullptr;
        DoubleSlider *slider = nullptr;
        QLabel *sliderValue = nullptr;
        QLabel *unitLabel = nullptr;
        QPushButton *currentValue = nullptr;
        QToolButton *commit = nullptr;
        QStringList toggleValues;
        QSet<QString> pendingValues;
        double sliderMinimum = 0;
        double sliderMaximum = 0;
        int sliderPrecision = 0;
        ActionInput input = ActionInput::Unavailable;
    };

    QMap<QString, Liveness> currentLiveness();
    void refreshTable();
    void refreshStatuses();
    void refreshActionControls();
    void refreshActionValues();
    QString actionInputValue(const ActionControl &control) const;
    void runSettingAction(const QString &groupName, const GroupAction &action,
                          const QString &value);
    void runBlockAction(const QString &groupName, const GroupAction &action);
    void membershipChanged(QTableWidgetItem *item);
    bool saveMembership(QString &error);

    QTabWidget *tabWidget = nullptr;
    QTableWidget *table = nullptr;
    QLabel *saveNotice = nullptr;
    QString configPath;
    QStringList groupNames;
    QMap<QString, QStringList> memberships;
    QMap<QString, QList<StatusRule>> statusRules;
    QMap<QString, QLabel *> noActiveWarningLabels;
    QMap<QString, QLabel *> blockWarningLabels;
    QMap<QString, QList<GroupAction>> groupActions;
    QList<ActionControl> actionControls;
    QMap<QString, QSet<QString>> messageFields;
    QMap<QString, QMap<QString, QString>> messageValues;
    QMap<QString, QStringList> messageEnums;
    QSet<QString> missingStatusBlockWarnings;
    QSet<QString> seenInTelemetry;
    QMap<QString, Liveness> displayedLiveness;
};

#endif // GROUP_CONTROLLER_H
