#include "Track.hh"

#include <game/system/RaceConfig.hh>
#include <game/ui/SectionManager.hh>

#include <cstdio>

namespace SP {

// clang-format off

constexpr Registry::Course getRaceCourseId(u8 slotId) {
    using enum Registry::Course;

    switch (slotId) {
    case 11: return LuigiCircuit;
    case 12: return MooMooMeadows;
    case 13: return MushroomGorge;
    case 14: return ToadsFactory;
    case 21: return MarioCircuit;
    case 22: return CoconutMall;
    case 23: return DKSummit;
    case 24: return WarioGoldMine;
    case 31: return DaisyCircuit;
    case 32: return KoopaCape;
    case 33: return MapleTreeway;
    case 34: return GrumbleVolcano;
    case 41: return DryDryRuins;
    case 42: return MoonviewHighway;
    case 43: return BowsersCastle;
    case 44: return RainbowRoad;
    case 51: return GCNPeachBeach;
    case 52: return DSYoshiFalls;
    case 53: return SNESGhostValley2;
    case 54: return N64MarioRaceway;
    case 61: return N64SherbetLand;
    case 62: return GBAShyGuyBeach;
    case 63: return DSDelfinoSquare;
    case 64: return GCNWaluigiStadium;
    case 71: return DSDesertHills;
    case 72: return GBABowserCastle3;
    case 73: return N64DKsJungleParkway;
    case 74: return GCNMarioCircuit;
    case 81: return SNESMarioCircuit3;
    case 82: return DSPeachGardens;
    case 83: return GCNDKMountain;
    case 84: return N64BowsersCastle;
    default: panic("Unknown race slot ID: %hhu", slotId);
    }
}

constexpr Registry::Course getBattleCourseId(u8 slotId) {
    using enum Registry::Course;

    switch (slotId) {
    case 11: return BlockPlaza;
    case 12: return DelfinoPier;
    case 13: return ChainChompRoulette;
    case 14: return FunkyStadium;
    case 15: return ThwompDesert;
    case 21: return SNESBattleCourse4;
    case 22: return GBABattleCourse3;
    case 23: return N64Skyscraper;
    case 24: return GCNCookieLand;
    case 25: return DSTwilightHouse;
    default: panic("Unknown battle slot ID: %hhu", slotId);
    }
}

// clang-format on

constexpr Registry::Course getCourseId(u8 slotId, bool isArena) {
    if (isArena) {
        return getBattleCourseId(slotId);
    } else {
        return getRaceCourseId(slotId);
    }
}

Track::Track(Sha1 sha1, u8 slotId, bool isArena, const char *name) {
    m_sha1 = sha1;
    m_musicId = std::nullopt;
    m_courseId = getCourseId(slotId, isArena);

    auto nameSize = strnlen(name, 0x48);
    m_name.setUTF8(std::string_view(name, nameSize));
}

void Track::applyToConfig(System::RaceConfig *raceConfig, bool inRace) const {
    auto *globalContext = UI::SectionManager::Instance()->globalContext();
    if (globalContext->isVanillaTracks()) {
        return;
    }

    System::RaceConfig::Scenario *scenario;
    System::RaceConfig::SPScenario *spScenario;
    if (inRace) {
        spScenario = &raceConfig->m_spRace;
        scenario = &raceConfig->m_raceScenario;
    } else {
        spScenario = &raceConfig->m_spMenu;
        scenario = &raceConfig->m_menuScenario;
    }

    spScenario->courseSha = m_sha1;
    spScenario->nameReplacement = m_name;
    spScenario->musicReplacement = m_musicId;
    scenario->courseId = m_courseId;

    auto hex = sha1ToHex(m_sha1);
    spScenario->pathReplacement.m_len = snprintf(spScenario->pathReplacement.m_buf.data(), 64,
            "Tracks/%s.arc.lzma", hex.data());
}

} // namespace SP
