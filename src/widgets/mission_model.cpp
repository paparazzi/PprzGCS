#include "mission_model.h"

void MissionInfo::setupGotoButton(MissionModel *parent)
{
    QPushButton *goto_button = new QPushButton("GOTO", parent);
    goto_button->setIcon(parent->style()->standardIcon(QStyle::SP_ArrowLeft));
    goto_button->setToolTip("Set this mission as first");
    goto_button->connect(goto_button, &QPushButton::clicked, [=, this]()
    {
        auto goto_definition = PprzDispatcher::get()->getDict()->getDefinition("GOTO_MISSION");
        pprzlink::Message msg(goto_definition);
        msg.addField("ac_id", parent->ac_id.toUInt());
        msg.addField("mission_id", id);
        PprzDispatcher::get()->sendMessage(msg);
    });

    parent->setItemWidget(main_item, 4, goto_button);
}

MissionInfo::MissionInfo(MissionModel *parent, uint8_t _id, uint8_t _rank)
: id(_id), rank(_rank)
{
    main_item = new QTreeWidgetItem(parent);
    main_item->setText(0, QString::number(rank));
    main_item->setText(1, "UNKNOWN");
    main_item->setText(2, QString::number(id));
    main_item->setText(3, "--");
    setupGotoButton(parent);
}

MissionInfo::MissionInfo(MissionModel *parent, uint8_t _id, uint8_t _rank, pprzlink::Message _mission_definition_message)
: id(_id), rank(_rank), mission_definition_message(_mission_definition_message)
{
    main_item = new QTreeWidgetItem(parent);
    main_item->setText(0, QString::number(rank));
    main_item->setText(2, QString::number(id));
    setMissionDefinitionMessage(_mission_definition_message);
    setupGotoButton(parent);
}

MissionModel::MissionModel(QString ac_id, QWidget *parent) : QTreeWidget(parent), ac_id(ac_id)
{
    auto charWidth = this->fontMetrics().averageCharWidth();

    this->setColumnCount(5);
    this->setHeaderLabels({"Rank", "Name", "Id/Value", "Time (s)", ""});
    this->resizeColumnToContents(0);
    this->setColumnWidth(1, charWidth * 18); // Set name column width to fit "MISSION_GOTO_WP_LLA" (trial and error guessed value)
    this->setColumnWidth(2, charWidth * 15); // Set value column width
    this->resizeColumnToContents(3);
    this->sortItems(0, Qt::AscendingOrder);

    PprzDispatcher::get()->bind("MISSION_STATUS", this, [=, this](QString sender, pprzlink::Message msg)
    {
        if(sender == ac_id) 
        {
            float remaining_time;
            QList<uint8_t> index_list;

            msg.getField("remaining_time", remaining_time);
            msg.getField("index_list", index_list);

            this->updateActiveMissions(remaining_time, index_list);
        } });

    for (auto &name : MISSION_MSG_NAMES)
    {
        PprzDispatcher::get()->bind(name, this, [=, this]([[maybe_unused]] QString sender, pprzlink::Message msg)
                                    {
            uint8_t dest_id;
            msg.getField("ac_id", dest_id);
            if(QString::number(dest_id) == ac_id) 
            {
                uint8_t mission_id;
                msg.getField("index", mission_id);
                if(this->missions.contains(mission_id))
                {
                    MissionInfo *info = this->missions[mission_id];
                    info->setMissionDefinitionMessage(msg);
                } 
                else
                {
                    MissionInfo* info = new MissionInfo(this,mission_id, active_missions_count, msg);
                    this->missions[mission_id] = info;
                    active_missions_count++;
                    this->addTopLevelItem(info->getMainItem());
                }
                active_missions.insert(mission_id);
            } });
    }
}

void MissionModel::updateActiveMissions(float remaining_time, const QList<uint8_t> &missions)
{
    QSet<uint8_t> new_active_missions(missions.constBegin(), missions.constEnd());

    QSet<uint8_t> deactivated_missions = active_missions - new_active_missions;

    for (uint8_t mission_id : deactivated_missions)
    {
        if (this->missions.contains(mission_id))
        {
            MissionInfo *info = this->missions[mission_id];
            info->setActive(false);
        }
    }

    uint8_t rank = 0;
    for (uint8_t mission_id : missions)
    {
        if (this->missions.contains(mission_id))
        {
            MissionInfo *info = this->missions[mission_id];
            info->setActive(true);
            info->setRank(rank);
            if (rank == 0)
            {
                info->setDuration(remaining_time);
            }
        }
        else
        {
            MissionInfo *info = new MissionInfo(this, mission_id, rank);
            info->setActive(true);
            this->missions[mission_id] = info;
            if (rank == 0)
            {
                info->setDuration(remaining_time);
            }
            this->addTopLevelItem(info->getMainItem());
        }
        rank++;
    }
    active_missions_count = rank;
    this->sortItems(0, Qt::AscendingOrder);
    active_missions = new_active_missions;
}