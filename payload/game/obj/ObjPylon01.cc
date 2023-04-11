#include "ObjPylon01.hh"

#include <game/system/RaceConfig.hh>

namespace Obj {

u32 Pylon01::vf_b0() {
    auto &raceScenario = System::RaceConfig::Instance()->raceScenario();
    auto firstPlayerType = raceScenario.players[0].type;

    if (firstPlayerType == System::RaceConfig::Player::Type::Ghost && m_playerId != 0) {
        return 1; // Transparent
    }

    return 12; // Opaque
}

} // namespace Obj
