/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ScriptMgr.h"
#include "Creature.h"
#include "Config.h"
#include "Chat.h"
#include "ScriptedGossip.h"
#include "Mail.h"
#include "AchievementMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Item.h"
#include "MapMgr.h"
#include "WorldSession.h"

uint32 creatureId;
Position* pos;
Map* map;

bool ENABLE_MODULE;
bool Q_POLLUTION;
bool Q_SEWER;

//General NPC scripts
class QuestPack_NPC : public CreatureScript
{
public:
    QuestPack_NPC() : CreatureScript("questpack_npc_script") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {

        if ((creature->GetEntry() == 200003) && (!player->HasSpell(30156)) && player->HasAchieved(953))
        {
            player->PlayerTalkClass->ClearMenus();
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I heard you may have a critter that needs a home?", creature->GetEntry(), 0);
            SendGossipMenuFor(player, 197761, creature->GetGUID());
        }
        else
        {
            SendGossipMenuFor(player, 197761, creature->GetGUID());
        }

        return true;
    }


    bool OnGossipSelect(Player* player, Creature* creature, uint32 Sender, uint32 areaid)
    {
        uint32 creatureID = creature->GetEntry();
        
        switch (creatureID)
        {
        case 200003:
            player->PlayerTalkClass->ClearMenus();
            SendGossipMenuFor(player, 197762, creature->GetGUID());
            player->AddItem(23713, 1);
            break;

        }
        return true;
    }

};


class Questpack_Config : public WorldScript
{
public:
    Questpack_Config() : WorldScript("questpack_config_script") {}

    void OnBeforeConfigLoad(bool) override
    {
        //config reads
        ENABLE_MODULE = sConfigMgr->GetOption<bool>("CQP.Enable", true);
        Q_POLLUTION = sConfigMgr->GetOption<bool>("CQP.Pollution", true);
        Q_SEWER = sConfigMgr->GetOption<bool>("CQP.Sewer", true);
        

        //Check and load spawns for quest NPCs and Objects
        

    }

    void OnStartup() override
    {
        LoadQuestSpawns();
        
    }

    void LoadQuestSpawns()
    {
        
        //Cleaning the Pollution Quest
        if (Q_POLLUTION)
        {
           creatureId = 200003;
           map = sMapMgr->FindMap(530, 0);
           pos = new Position(-200.818, 5560.27, 24.0272, 4.5);
           AddQuestNPCs(creatureId, map, pos);
        }
        else
        {
            RemoveQuestNPCs(200003);
        }

        //Sewer Cretures Quest
        if (Q_SEWER)
        {
            creatureId = 200004;
            map = sMapMgr->FindMap(571, 0);
            pos = new Position(5649.42, 861.011, 571.393, 4);
            AddQuestNPCs(creatureId, map, pos);
            
            creatureId = 3581;
            map = sMapMgr->FindMap(0, 0);
            pos = new Position(1759.35, 455.367, -69.8927, 3.22);
            AddQuestNPCs(creatureId, map, pos);


        }
        else
        {
            RemoveQuestNPCs(200004);
            //Not removing extra sewer beast since it really doesn't matter.
        }
        
        
    }


    void AddQuestNPCs(uint32 creatureId, Map* map, Position* pos)
    {
        QueryResult result;
        result = WorldDatabase.Query("SELECT guid, position_x, position_y, position_z, map FROM creature WHERE id1='{}' OR id2='{}' OR id3='{}'",
            uint32(creatureId), uint32(creatureId), uint32(creatureId));
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                ObjectGuid::LowType guid = fields[0].Get<uint32>();
                float x = fields[1].Get<float>();
                float y = fields[2].Get<float>();
                float z = fields[3].Get<float>();
                uint16 mapId = fields[4].Get<uint16>();

                if (!mapId == map->GetId())
                {
                    Creature* creature = new Creature();
                    creature->Create(map->GenerateLowGuid<HighGuid::Unit>(), map, 255, 200003, 0, pos->GetPositionX(), pos->GetPositionY(), pos->GetPositionZ(), pos->GetOrientation());
                    creature->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), 255);
                    ObjectGuid::LowType spawnId = creature->GetSpawnId();
                    creature->CleanupsBeforeDelete();
                    delete creature;
                    creature = new Creature();
                    creature->LoadCreatureFromDB(spawnId, map, true, false, true);
                    sObjectMgr->AddCreatureToGrid(spawnId, sObjectMgr->GetCreatureData(spawnId));
                }
               
            } while (result->NextRow());
        }
        else
        {
            Creature* creature = new Creature();
            creature->Create(map->GenerateLowGuid<HighGuid::Unit>(), map, 255, creatureId, 0, pos->GetPositionX(), pos->GetPositionY(), pos->GetPositionZ(), pos->GetOrientation());
            creature->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), 255);
            ObjectGuid::LowType spawnId = creature->GetSpawnId();
            creature->CleanupsBeforeDelete();
            delete creature;
            creature = new Creature();
            creature->LoadCreatureFromDB(spawnId, map, true, false, true);
            sObjectMgr->AddCreatureToGrid(spawnId, sObjectMgr->GetCreatureData(spawnId));
        }
    }

    void RemoveQuestNPCs(uint32 creatureId)
    {
        QueryResult result;
        result = WorldDatabase.Query("SELECT guid FROM creature WHERE id1='{}' OR id2='{}' OR id3='{}'",
            uint32(creatureId), uint32(creatureId), uint32(creatureId));
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                ObjectGuid::LowType guid = fields[0].Get<uint32>();
                float x = fields[1].Get<float>();
                float y = fields[2].Get<float>();
                float z = fields[3].Get<float>();
                uint16 mapId = fields[4].Get<uint16>();

                sObjectMgr->RemoveCreatureFromGrid(guid, sObjectMgr->GetCreatureData(guid));
                


            } while (result->NextRow());
        }


    }


};

class QuestPack_Player : public PlayerScript
{
public:
    QuestPack_Player() : PlayerScript("questpack_player_script") {}

   
    void OnLogin(Player* player) override
    {
        if (!ENABLE_MODULE)
        {
            return;
        }
        //Cleaning up the Pollution setup 
        if (player->HasAchieved(953) && (player->GetQuestStatus(30001) != QUEST_STATUS_REWARDED) && Q_POLLUTION)
        {

            player->AddQuest(sObjectMgr->GetQuestTemplate(30001), nullptr);
            player->CompleteQuest(30001);
            player->RewardQuest(sObjectMgr->GetQuestTemplate(30001), 0, nullptr, true, false);

            std::string subject;
            std::string body;
            subject = "Help Duridz lol";
            body = "Duridz is need help lol. Find in Zanra... Zangen... Zarish.... uhhh, big mushroom swamp.";

          CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
          MailDraft(subject, body).AddItem(Item::CreateItem(23713, 1, nullptr, false, 0)).SendMailTo(trans, player, MailSender(200003), MAIL_CHECK_MASK_NONE, 0U, 0U, false, true);
          CharacterDatabase.CommitTransaction(trans);
        }

    }


};

// Add all scripts in one
void AddMyScripts()
{
    new Questpack_Config();
    new QuestPack_NPC();
    new QuestPack_Player();
}
