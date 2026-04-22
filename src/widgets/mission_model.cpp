#include "mission_model.h"

MissionModel::MissionModel(QString ac_id, QWidget *parent) : QTreeWidget(parent), ac_id(ac_id)
{
    auto charWidth = this->fontMetrics().averageCharWidth();

    this->setColumnCount(4);
    this->setHeaderLabels({"Index", "Name", "Id/Value", "Duration (s)"});
    this->resizeColumnToContents(0);
    this->setColumnWidth(1, charWidth * 18); // Set name column width to fit "MISSION_GOTO_WP_LLA" (trial and error guessed value)
    this->setColumnWidth(2, charWidth * 20); // Set value column width
    this->resizeColumnToContents(3); 
    this->sortItems(0, Qt::AscendingOrder);

    PprzDispatcher::get()->bind("MISSION_STATUS", this, [=,this](QString sender, pprzlink::Message msg)
    {
        if(sender == ac_id) 
        {
            float remaining_time;
            QList<uint8_t> index_list;

            msg.getField("remaining_time", remaining_time);
            msg.getField("index_list", index_list);

            this->updateActiveMissions(remaining_time, index_list);
        } });

    for(auto& name : MISSION_MSG_NAMES)
    {
        PprzDispatcher::get()->bind(name, this, [=,this]([[maybe_unused]] QString sender, pprzlink::Message msg)
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
            } 
        });
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