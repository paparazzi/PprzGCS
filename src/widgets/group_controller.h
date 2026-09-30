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

class GroupController : public QWidget
{
    Q_OBJECT

public:
    explicit GroupController(QWidget *parent = nullptr);

private:
    enum class Liveness { Active, Lost, Offline };
    struct StatusRule {
        QString title;
        QString source;
        QString expected;
        QString comparison;
        QString messageName;
        QString fieldName;
        bool isSetting = false;
        QLabel *label = nullptr;
    };
    struct SettingAction {
        QString title;
        QString variable;
        QString value;
    };

    QMap<QString, Liveness> currentLiveness();
    void refreshTable();
    void refreshStatuses();
    void runSettingAction(const QString &groupName, const SettingAction &action);
    void membershipChanged(QTableWidgetItem *item);
    bool saveMembership(const QString &groupName, const QString &aircraftName,
                        bool member, QString &error);

    QTabWidget *tabWidget = nullptr;
    QTableWidget *table = nullptr;
    QString configPath;
    QStringList groupNames;
    QMap<QString, QStringList> memberships;
    QMap<QString, QList<StatusRule>> statusRules;
    QMap<QString, QList<SettingAction>> settingActions;
    QMap<QString, QSet<QString>> messageFields;
    QMap<QString, QMap<QString, QString>> messageValues;
    QMap<QString, QStringList> messageEnums;
    QSet<QString> seenInTelemetry;
    QMap<QString, Liveness> displayedLiveness;
};

#endif // GROUP_CONTROLLER_H
