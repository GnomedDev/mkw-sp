#pragma once

#include "sp/FixedString.hh"
#include "sp/ShaUtil.hh"

#include <limits>

namespace SP {

class Track {
public:
    enum class Mode {
        Race = 1 << 0,
        Balloon = 1 << 1,
        Coin = 1 << 2,
    };

    Track(Sha1 sha1, u8 slotId, bool isArena, const char *name);

    u32 getCourseId() const;

    Sha1 m_sha1;
    u8 m_slotId = 0;
    bool m_isArena = false;
    u16 m_musicId = std::numeric_limits<u16>::max();
    WFixedString<48> m_name = {};

private:
    u32 getRaceCourseId() const;
    u32 getBattleCourseId() const;
};

static_assert(sizeof(Track) == 0x7c);

constexpr Track::Mode modes[] = {
        Track::Mode::Race,
        Track::Mode::Coin,
        Track::Mode::Balloon,
};

} // namespace SP
