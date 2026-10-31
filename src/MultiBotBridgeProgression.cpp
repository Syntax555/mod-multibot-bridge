#include "MultiBotBridgeProgression.h"

#include "ConditionMgr.h"
#include "DBCStores.h"
#include "DisableMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "World.h"

#include <map>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#if __has_include("IndividualProgression.h")
#include "IndividualProgression.h"
#include "StrictEarnedProgressionPolicy.h"
#define MBB_WITH_INDIVIDUAL_PROGRESSION 1
#endif

// Native checks run without client messages; nothing is granted, changed or
// written. A realm without the module reports ip=0 and no progression gates.
namespace
{
// Acceptance checks of Player::CanTakeQuest that fail, plus giver/source access.
enum IpQuestReason : uint32
{
    IP_QUEST_DISABLED          = 0x000001,
    IP_QUEST_CLASS             = 0x000002,
    IP_QUEST_RACE              = 0x000004,
    IP_QUEST_LEVEL_LOW         = 0x000008,
    IP_QUEST_LEVEL_HIGH        = 0x000010,
    IP_QUEST_SKILL             = 0x000020,
    IP_QUEST_REPUTATION        = 0x000040,
    IP_QUEST_PREVIOUS          = 0x000080,
    IP_QUEST_TIMED             = 0x000100,
    IP_QUEST_EXCLUSIVE         = 0x000200,
    IP_QUEST_NEXT_CHAIN        = 0x000400,
    IP_QUEST_PREV_CHAIN        = 0x000800,
    IP_QUEST_BREADCRUMB_DONE   = 0x001000,
    IP_QUEST_BREADCRUMB_ACTIVE = 0x002000,
    IP_QUEST_REPEAT            = 0x004000,
    IP_QUEST_CONDITIONS        = 0x008000,
    IP_QUEST_TIER              = 0x010000,
    IP_QUEST_GIVER_HIDDEN      = 0x020000, // Also a forbidden target quest-source map.
    IP_QUEST_LOG_FULL          = 0x040000,
};

#ifdef MBB_WITH_INDIVIDUAL_PROGRESSION
using IpVisibilityRule = bool (*)(Player* target);

// The CanBeSeen rule of every IndividualProgressionAwareness script after its
// game master and disabled-module exceptions; TestIpProgressBridge compares them.
std::map<std::string, IpVisibilityRule, std::less<>> const& IpVisibilityRules()
{
    static std::map<std::string, IpVisibilityRule, std::less<>> const rules = {
        {"gobject_ipp_preaq", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_BLACKWING_LAIR); }},
        {"gobject_ipp_aqwar", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_PRE_AQ) && !sIndividualProgression->hasPassedProgression(target, PROGRESSION_AQ_WAR); }},
        {"gobject_ipp_si", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_AQ) && !sIndividualProgression->hasPassedProgression(target, PROGRESSION_NAXX40); }},
        {"gobject_ipp_naxx40", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_AQ); }},
        {"gobject_ipp_pre_tbc", [](Player* target) { return sIndividualProgression->isBeforeProgression(target, PROGRESSION_PRE_TBC); }},
        {"gobject_ipp_tbc", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_PRE_TBC); }},
        {"gobject_ipp_tbc_t4", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_TBC_TIER_4); }},
        {"gobject_ipp_wotlk", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_TBC_TIER_5); }},
        {"npc_ipp_preaq", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_BLACKWING_LAIR); }},
        {"npc_ipp_zg", [](Player* target) { return sIndividualProgression->GetPlayerProgressionFromQuests(target) >= static_cast<ProgressionState>(sIndividualProgression->RequiredZulGurubProgression); }},
        {"npc_ipp_we", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_BLACKWING_LAIR) && !sIndividualProgression->hasPassedProgression(target, PROGRESSION_PRE_AQ); }},
        {"npc_ipp_aq", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_AQ_WAR); }},
        {"npc_ipp_aqwewar", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_BLACKWING_LAIR) && !sIndividualProgression->hasPassedProgression(target, PROGRESSION_AQ_WAR); }},
        {"npc_ipp_aqwar", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_PRE_AQ) && !sIndividualProgression->hasPassedProgression(target, PROGRESSION_AQ_WAR); }},
        {"npc_ipp_si", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_AQ) && !sIndividualProgression->hasPassedProgression(target, PROGRESSION_NAXX40); }},
        {"npc_ipp_pre_naxx40", [](Player* target) { return sIndividualProgression->isBeforeProgression(target, PROGRESSION_AQ); }},
        {"npc_ipp_naxx40", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_AQ) || sIndividualProgression->isExcludedAccount(target); }},
        {"npc_ipp_pre_tbc", [](Player* target) { return sIndividualProgression->isBeforeProgression(target,PROGRESSION_PRE_TBC); }},
        {"npc_ipp_tbc", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_PRE_TBC); }},
        {"npc_ipp_tbc_pre_t3", [](Player* target) { return !sIndividualProgression->hasPassedProgression(target,PROGRESSION_TBC_TIER_2); }},
        {"npc_ipp_tbc_t3", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_TBC_TIER_2); }},
        {"npc_ipp_tbc_t4", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_TBC_TIER_4); }},
        {"npc_ipp_za", [](Player* target) { return sIndividualProgression->GetPlayerProgressionFromQuests(target) >= static_cast<ProgressionState>(sIndividualProgression->RequiredZulAmanProgression); }},
        {"npc_ipp_pre_wotlk", [](Player* target) { return sIndividualProgression->isBeforeProgression(target, PROGRESSION_TBC_TIER_5); }},
        {"npc_ipp_wotlk", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_TBC_TIER_5); }},
        {"npc_ipp_wotlk_ulduar", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_WOTLK_TIER_1); }},
        {"npc_ipp_wotlk_totc", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_WOTLK_TIER_2); }},
        {"npc_ipp_wotlk_icc", [](Player* target) { return sIndividualProgression->hasPassedProgression(target, PROGRESSION_WOTLK_TIER_3); }},
        {"npc_ipp_ds2", [](Player* target) { return sIndividualProgression->earlyDungeonSet2 || sIndividualProgression->hasPassedProgression(target, PROGRESSION_BLACKWING_LAIR); }},
    };
    return rules;
}
#endif

struct IpGiverSpawn
{
    uint32 scriptId;
    uint32 phaseMask;
    uint32 mapId;
    float x, y, z;
};

// Quest starters and their spawns, indexed once from the loaded world data.
struct IpQuestGivers
{
    std::unordered_map<uint32, std::vector<uint32>> creatures;
    std::unordered_map<uint32, std::vector<uint32>> objects;
    std::unordered_map<uint32, std::vector<IpGiverSpawn>> creatureSpawns;
    std::unordered_map<uint32, std::vector<IpGiverSpawn>> objectSpawns;
};

IpQuestGivers const& GetIpQuestGivers()
{
    static IpQuestGivers const givers = []
    {
        IpQuestGivers result;
        for (auto const& [entry, quest] : *sObjectMgr->GetCreatureQuestRelationMap())
            result.creatures[quest].push_back(entry);
        for (auto const& [entry, quest] : *sObjectMgr->GetGOQuestRelationMap())
            result.objects[quest].push_back(entry);
        std::unordered_set<uint32> creatureStarters, objectStarters;
        for (auto const& [quest, entries] : result.creatures)
            creatureStarters.insert(entries.begin(), entries.end());
        for (auto const& [quest, entries] : result.objects)
            objectStarters.insert(entries.begin(), entries.end());
        // Creature::GetScriptId uses a spawn script only for the spawn's primary entry.
        for (auto const& [spawnId, data] : sObjectMgr->GetAllCreatureData())
            for (uint32 const entry : {data.id, data.id2, data.id3})
                if (entry && creatureStarters.contains(entry))
                    if (CreatureTemplate const* creature = sObjectMgr->GetCreatureTemplate(entry))
                        result.creatureSpawns[entry].push_back({data.ScriptId && entry == data.id ?
                            data.ScriptId : creature->ScriptID, data.phaseMask, data.mapid,
                            data.posX, data.posY, data.posZ});
        for (auto const& [spawnId, data] : sObjectMgr->GetAllGOData())
            if (objectStarters.contains(data.id))
                if (GameObjectTemplate const* object = sObjectMgr->GetGameObjectTemplate(data.id))
                    result.objectSpawns[data.id].push_back({data.ScriptId ? data.ScriptId : object->ScriptId,
                        data.phaseMask, data.mapid, data.posX, data.posY, data.posZ});
        return result;
    }();
    return givers;
}

// Quest-log categories do not locate a source; only loaded giver spawns do.
char DescribeQuestSourceAccess(Player* player, uint32 questId)
{
    IpQuestGivers const& givers = GetIpQuestGivers();
    bool known = false, unknown = false;
    auto accessible = [&](auto const& starters, auto const& spawns)
    {
        auto const found = starters.find(questId);
        if (found == starters.end())
            return false;
        for (uint32 const entry : found->second)
        {
            auto const entrySpawns = spawns.find(entry);
            if (entrySpawns == spawns.end())
            {
                unknown = true;
                continue;
            }
            for (IpGiverSpawn const& spawn : entrySpawns->second)
            {
                if (!sMapStore.LookupEntry(spawn.mapId))
                {
                    unknown = true;
                    continue;
                }
                known = true;
                if (!(spawn.phaseMask & PHASEMASK_NORMAL))
                {
                    unknown = true;
                    continue;
                }
#ifdef MBB_WITH_INDIVIDUAL_PROGRESSION
                if (sIndividualProgression->enabled && !player->IsGameMaster())
                {
                    auto const rule = IpVisibilityRules().find(sObjectMgr->GetScriptName(spawn.scriptId));
                    if (rule != IpVisibilityRules().end() && !rule->second(player))
                        continue;
                    if (!sIndividualProgression->CanEnterMap(player, spawn.mapId, spawn.x, spawn.y, spawn.z))
                        continue;
                }
#endif
                if (!sDisableMgr->IsDisabledFor(DISABLE_TYPE_MAP, spawn.mapId, player))
                    return true;
            }
        }
        return false;
    };
    if (accessible(givers.creatures, givers.creatureSpawns) || accessible(givers.objects, givers.objectSpawns))
        return 'A';
    return unknown || !known ? 'U' : 'L';
}

#ifdef MBB_WITH_INDIVIDUAL_PROGRESSION
// True only when every spawn of every quest starter is hidden from this
// character by an Individual Progression visibility script. Unspawned,
// phased and otherwise scripted givers count as visible.
bool IsIpQuestGiverHidden(Player* player, uint32 questId)
{
    IpQuestGivers const& givers = GetIpQuestGivers();
    bool hidden = false;
    auto allHidden = [&](auto const& starters, auto const& spawns)
    {
        auto const found = starters.find(questId);
        if (found == starters.end())
            return true;
        for (uint32 const entry : found->second)
        {
            auto const entrySpawns = spawns.find(entry);
            if (entrySpawns == spawns.end())
                return false;
            for (IpGiverSpawn const& spawn : entrySpawns->second)
            {
                auto const rule = IpVisibilityRules().find(sObjectMgr->GetScriptName(spawn.scriptId));
                if (!(spawn.phaseMask & PHASEMASK_NORMAL) || rule == IpVisibilityRules().end() || rule->second(player))
                    return false;
            }
            hidden = true;
        }
        return true;
    };
    return allHidden(givers.creatures, givers.creatureSpawns) && allHidden(givers.objects, givers.objectSpawns) && hidden;
}
#endif

// Entry checks a portal applies, without its raid-group, lockout and refusal
// side effects: '-' no such difficulty, 'D' disabled, 'G' Individual
// Progression refuses, 'R' an access requirement is missing, 'A' accessible.
char DescribeMapAccess(Player* player, uint32 mapId, Difficulty difficulty, bool silent)
{
    if (!GetMapDifficultyData(mapId, difficulty))
        return '-';
    // Player::Satisfy sends a refusal for a map disabled at the player's own difficulty.
    if (!silent || sDisableMgr->IsDisabledFor(DISABLE_TYPE_MAP, mapId, nullptr, difficulty))
        return 'D';
#ifdef MBB_WITH_INDIVIDUAL_PROGRESSION
    AreaTriggerTeleport const* const entrance = sObjectMgr->GetMapEntranceTrigger(mapId);
    if (sIndividualProgression->enabled && !sIndividualProgression->CanEnterMap(player, mapId,
            entrance ? entrance->target_X : 0.0f, entrance ? entrance->target_Y : 0.0f,
            entrance ? entrance->target_Z : 0.0f))
        return 'G';
#endif
    return player->Satisfy(sObjectMgr->GetAccessRequirement(mapId, difficulty), mapId, false) ? 'A' : 'R';
}
}

namespace MultiBotBridgeProgression
{
std::string State(Player* player)
{
    uint32 ip = 0, strict = 0, stored = 0, effective = 0, limit = 0, era = 255, next = 0, nextMet = 0;
    char account = 'N';
#ifdef MBB_WITH_INDIVIDUAL_PROGRESSION
    if (sIndividualProgression->enabled)
    {
        ip = 1;
        strict = sIndividualProgression->IsStrictDefaultProgression() ? 1 : 0;
        stored = sIndividualProgression->GetPlayerProgressionFromQuests(player);
        limit = sIndividualProgression->progressionLimit > 0 ? uint32(sIndividualProgression->progressionLimit) : 0;
        // hasPassedProgression refuses every stage above the realm limit.
        effective = limit && stored > limit ? limit : stored;
        era = IndividualProgression_GetEarnedEra(player);
        account = sIndividualProgression->isBotAccount(player) ? 'B' :
            sIndividualProgression->isExcludedAccount(player) ? 'X' : 'N';
        if (strict)
        {
            next = StrictEarnedProgressionPolicy::NextStage(uint8(stored));
            // Quest milestones read quest status. Boss credits live in a player
            // setting whose getter creates a missing entry, so they stay unread.
            if (next && !StrictEarnedProgressionPolicy::BossCredit(uint8(next)))
                nextMet = sIndividualProgression->HasStrictEarnedMilestone(player, uint8(next)) ? 1 : 0;
        }
    }
#endif
    std::ostringstream fields;
    fields << 1 << '~' << ip << '~' << strict << '~' << stored << '~' << effective << '~' << limit << '~' << era
           << '~' << next << '~' << nextMet << '~' << account << '~' << (player->IsGameMaster() ? 1 : 0)
           << '~' << sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
    return fields.str();
}

std::string Quest(Player* player, uint32 questId, bool forSource)
{
    std::string const id = std::to_string(questId) + ':';
    ::Quest const* const quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return id + 'U';
    if (player->GetQuestStatus(questId) == QUEST_STATUS_REWARDED)
        return id + 'R';
    if (forSource)
    {
        char const access = DescribeQuestSourceAccess(player, questId);
        if (access == 'U')
            return id + 'U';
        if (access == 'L')
            return id + "L20000";
    }
    switch (player->GetQuestStatus(questId))
    {
        case QUEST_STATUS_COMPLETE: return id + 'C';
        case QUEST_STATUS_INCOMPLETE: return id + 'I';
        case QUEST_STATUS_FAILED: return id + 'F';
        default: break;
    }

    uint32 reasons = 0;
    int32 tier = 0;
    if (sDisableMgr->IsDisabledFor(DISABLE_TYPE_QUEST, questId, player))
        reasons |= IP_QUEST_DISABLED;
    if (!player->SatisfyQuestClass(quest, false))
        reasons |= IP_QUEST_CLASS;
    if (!player->SatisfyQuestRace(quest, false))
        reasons |= IP_QUEST_RACE;
    if (player->GetLevel() < quest->GetMinLevel())
        reasons |= IP_QUEST_LEVEL_LOW;
    else if (!player->SatisfyQuestLevel(quest, false))
        reasons |= IP_QUEST_LEVEL_HIGH;
    if (!player->SatisfyQuestSkill(quest, false))
        reasons |= IP_QUEST_SKILL;
    if (!player->SatisfyQuestReputation(quest, false))
        reasons |= IP_QUEST_REPUTATION;
    if (!player->SatisfyQuestPreviousQuest(quest, false))
        reasons |= IP_QUEST_PREVIOUS;
    if (!player->SatisfyQuestTimed(quest, false))
        reasons |= IP_QUEST_TIMED;
    if (!player->SatisfyQuestExclusiveGroup(quest, false))
        reasons |= IP_QUEST_EXCLUSIVE;
    if (!player->SatisfyQuestNextChain(quest, false))
        reasons |= IP_QUEST_NEXT_CHAIN;
    if (!player->SatisfyQuestPrevChain(quest, false))
        reasons |= IP_QUEST_PREV_CHAIN;
    if (!player->SatisfyQuestBreadcrumb(quest, false))
    {
        // A breadcrumb whose target is taken never returns; an active breadcrumb ends on turn-in.
        uint32 const target = quest->GetBreadcrumbForQuestId();
        reasons |= target && (player->GetQuestStatus(target) != QUEST_STATUS_NONE || player->IsQuestRewarded(target)) ?
            IP_QUEST_BREADCRUMB_DONE : IP_QUEST_BREADCRUMB_ACTIVE;
    }
    if (!player->SatisfyQuestDay(quest, false) || !player->SatisfyQuestWeek(quest, false) ||
        !player->SatisfyQuestMonth(quest, false) || !player->SatisfyQuestSeasonal(quest, false))
        reasons |= IP_QUEST_REPEAT;
    if (!player->SatisfyQuestConditions(quest, false))
    {
        reasons |= IP_QUEST_CONDITIONS;
        // Name the first failing progression marker row: +n needs tier n, -n was retired by it.
        ConditionSourceInfo source(player);
        for (Condition* condition : sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_QUEST_AVAILABLE, questId))
            if (condition->ConditionType == CONDITION_QUESTREWARDED && condition->ConditionValue1 > 66000 &&
                condition->ConditionValue1 <= 66018 && !condition->Meets(source))
            {
                reasons |= IP_QUEST_TIER;
                tier = int32(condition->ConditionValue1 - 66000) * (condition->NegativeCondition ? -1 : 1);
                break;
            }
    }
#ifdef MBB_WITH_INDIVIDUAL_PROGRESSION
    if (sIndividualProgression->enabled && !player->IsGameMaster() && IsIpQuestGiverHidden(player, questId))
        reasons |= IP_QUEST_GIVER_HIDDEN;
#endif
    if (!player->SatisfyQuestLog(false))
        reasons |= IP_QUEST_LOG_FULL;
    if (!reasons)
        return id + 'A';

    std::ostringstream locked;
    locked << id << 'L' << std::hex << reasons << std::dec;
    if (tier)
        locked << ':' << (tier > 0 ? "+" : "") << tier;
    return locked.str();
}

std::vector<std::string> Access(Player* player)
{
    std::vector<std::string> items;
    for (uint32 mapId = 0; mapId < sMapStore.GetNumRows(); ++mapId)
    {
        MapEntry const* const map = sMapStore.LookupEntry(mapId);
        if (!map || !map->IsDungeon() || !sObjectMgr->GetInstanceTemplate(mapId))
            continue;
        bool const silent = !sDisableMgr->IsDisabledFor(DISABLE_TYPE_MAP, mapId, player);
        std::string item = std::to_string(mapId) + ':';
        for (uint8 difficulty = 0; difficulty < MAX_DIFFICULTY; ++difficulty)
            item += DescribeMapAccess(player, mapId, Difficulty(difficulty), silent);
        items.push_back(std::move(item));
    }
    return items;
}
}
