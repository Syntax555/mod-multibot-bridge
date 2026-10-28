#ifndef MULTIBOT_BRIDGE_PROGRESSION_H
#define MULTIBOT_BRIDGE_PROGRESSION_H

#include "Define.h"

#include <string>
#include <vector>

class Player;

// Read-only Individual Progression answers about the supplied character
// for IP_PROGRESS_V1 and IP_TARGET_V1. Individual Progression's header cannot share a
// translation unit with Playerbots' (both declare GENERAL), so it lives here.
namespace MultiBotBridgeProgression
{
    // The IP_STATE fields after the token; an unchanged state formats equally.
    std::string State(Player* player);
    // "<quest>:<status>[<hex reasons>][:<tier>]" for one IP_QUESTS entry.
    std::string Quest(Player* player, uint32 questId, bool forSource = false);
    // "<map>:<access for each difficulty>" for every instance map, for IP_ACCESS.
    std::vector<std::string> Access(Player* player);
}

#endif
