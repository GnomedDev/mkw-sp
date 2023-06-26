#pragma once

#include "sp/FixedString.hh"
#include "sp/ShaUtil.hh"

#include <game/sound/SoundId.hh>
#include <game/util/Registry.hh>

#include <limits>
#include <optional>

namespace System {
class RaceConfig;
}

namespace SP {

class Track {
public:
    enum class Mode {
        Race = 1 << 0,
        Balloon = 1 << 1,
        Coin = 1 << 2,
    };

    Track(Sha1 sha1, u8 slotId, bool isArena, const char *name);
    void applyToConfig(System::RaceConfig *raceConfig, bool inRace) const;

    Sha1 m_sha1;
    Registry::Course m_courseId;
    std::optional<Sound::SoundId> m_musicId;
    WFixedString<48> m_name = {};
};

constexpr Track::Mode s_trackModes[] = {
        Track::Mode::Race,
        Track::Mode::Coin,
        Track::Mode::Balloon,
};

} // namespace SP
