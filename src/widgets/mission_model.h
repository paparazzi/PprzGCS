#ifndef MISSION_MODEL_H
#define MISSION_MODEL_H

#include <QtWidgets>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <block.h>
#include <memory>
#include <functional>
#include <optional>
#include <array>
#include "pprz_dispatcher.h"

static const std::array<QString, 9> MISSION_MSG_NAMES = {
        "MISSION_GOTO_WP",
        "MISSION_GOTO_WP_LLA",
        "MISSION_CIRCLE",
        "MISSION_CIRCLE_LLA",
        "MISSION_SEGMENT",
        "MISSION_SEGMENT_LLA",
        "MISSION_PATH",
        "MISSION_PATH_LLA",
        "MISSION_CUSTOM"
    };

class MissionInfo;

class MissionModel : public QTreeWidget
{
    Q_OBJECT
public:
    QString ac_id;
    explicit MissionModel(QString ac_id, QWidget *parent = nullptr);

signals:

public slots:

private:
    uint8_t active_missions_count = 0;

    QString labels_stylesheet;

    QSet<uint8_t> active_missions = QSet<uint8_t>(); // Set of active mission ids
    QMap<uint8_t,MissionInfo*> missions = QMap<uint8_t,MissionInfo*>(); // Map from mission id to mission info

    void updateActiveMissions(float remaining_time, const QList<uint8_t> &missions);
    
};

class MissionInfo
{
private:
    uint8_t id,rank;
    optional<float> duration = nullopt;
    optional<pprzlink::Message> mission_definition_message = nullopt;
    bool active = false;

    QTreeWidgetItem* main_item;
    QMap<QString, QTreeWidgetItem*> details = QMap<QString, QTreeWidgetItem*>();

    void setupGotoButton(MissionModel* parent);

public:
    // ----- Constructors -----
    MissionInfo() = delete;
    MissionInfo(MissionModel* parent, uint8_t _id, uint8_t _rank);
    MissionInfo(MissionModel* parent, uint8_t _id, uint8_t _rank, pprzlink::Message _mission_definition_message);

    // ----- Getters -----
    uint8_t getId() const { return id; }
    uint8_t getRank() const { return rank; }
    optional<float> getDuration() const { return duration; }
    optional<pprzlink::Message> getMissionDefinitionMessage() const { return mission_definition_message; }
    bool isActive() const { return active; }

    QTreeWidgetItem* getMainItem() const
    {
        return main_item;
    }

    // ----- Setters -----
    void setRank(uint8_t _rank)
    {
        rank = _rank;
        main_item->setText(0, QString::number(rank));
    }

    void setName(const QString& name)
    {
        main_item->setText(1, name);
    }

    void setId(uint8_t _id)
    {
        id = _id;
        main_item->setText(2, QString::number(id));
    }

    void setDuration(float _duration)
    {   
        duration = _duration;
        main_item->setText(3, QString::number(_duration, 'f', 3));
    }

    void setMissionDefinitionMessage(pprzlink::Message _mission_definition_message)
    {
        mission_definition_message = _mission_definition_message;
        main_item->setText(1, _mission_definition_message.getDefinition().getName());
        float _dval;
        _mission_definition_message.getField("duration", _dval);
        setDuration(_dval);
        duration.emplace(_dval);

        for(size_t i = 0; i < _mission_definition_message.getDefinition().getNbFields(); i++)
        {
            auto field = _mission_definition_message.getDefinition().getField(i);
            QString field_name = field.getName();
            if (field_name == "duration" || field_name == "ac_id" || field_name == "index") continue; // Already displayed

            QTreeWidgetItem* field_item;
            if (!details.contains(field_name))
            {
                field_item = new QTreeWidgetItem(main_item);
            }
            else
            {
                field_item = details[field_name];
                
            }
            field_item->setText(1, field_name);
            field_item->setText(2, _mission_definition_message.getFieldAsStr(i));
            field_item->setToolTip(2, _mission_definition_message.getFieldAsStr(i));
            details[field_name] = field_item;
        }
    }

    void setActive(bool _active)
    {   
        active = _active;
        main_item->setHidden(!_active);
        main_item->setDisabled(!_active);
    }
};


class MissionWidget : public QWidget
{
    Q_OBJECT
public:
    QString ac_id;
    explicit MissionWidget(QString _ac_id, QWidget *parent = nullptr)
    : QWidget(parent), ac_id(_ac_id)
    {
        layout = new QVBoxLayout(this);
        layout->addWidget(buildNextMissionButton());
        layout->addWidget(buildEndMissionButton());

        auto mission_model = new MissionModel(ac_id, this);
        layout->addWidget(mission_model);
        
    }

private:
    QVBoxLayout *layout;

    QPushButton * buildEndMissionButton()
    {
        auto button = new QPushButton("End Mission", this);
        button->setIcon(this->style()->standardIcon(QStyle::SP_DialogCloseButton));
        button->setToolTip("End all missions");
        connect(button, &QPushButton::clicked, this, [=]()
        {
            pprzlink::Message msg(PprzDispatcher::get()->getDict()->getDefinition("END_MISSION"));
            msg.addField("ac_id", ac_id.toUInt());
            PprzDispatcher::get()->sendMessage(msg);
        });
        return button;
    }

    QPushButton * buildNextMissionButton()
    {
        auto button = new QPushButton("Next Mission", this);
        button->setIcon(this->style()->standardIcon(QStyle::SP_ArrowRight));
        button->setToolTip("Skip to next mission");
        connect(button, &QPushButton::clicked, this, [=]()
        {
            pprzlink::Message msg(PprzDispatcher::get()->getDict()->getDefinition("NEXT_MISSION"));
            msg.addField("ac_id", ac_id.toUInt());
            PprzDispatcher::get()->sendMessage(msg);
        });
        return button;
    }
};

#endif // MISSION_MODEL_H