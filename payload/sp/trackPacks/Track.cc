#include "Track.hh"

namespace SP {

Track::Track(Sha1 sha1, u8 slotId, bool isArena, const char *name) {
    m_sha1 = sha1;
    m_slotId = slotId;
    m_isArena = isArena;

    auto nameSize = strnlen(name, 0x48);
    m_name.setUTF8(std::string_view(name, nameSize));
}

u32 Track::getCourseId() const {
    if (m_isArena) {
        return getBattleCourseId();
    } else {
        return getRaceCourseId();
    }
}

// clang-format off
u32 Track::getRaceCourseId() const {
    switch (m_slotId) {
    case 11: return 0x8;
    case 12: return 0x1;
    case 13: return 0x2;
    case 14: return 0x4;
    case 21: return 0x0;
    case 22: return 0x5;
    case 23: return 0x6;
    case 24: return 0x7;
    case 31: return 0x9;
    case 32: return 0xF;
    case 33: return 0xB;
    case 34: return 0x3;
    case 41: return 0xE;
    case 42: return 0xA;
    case 43: return 0xC;
    case 44: return 0xD;
    case 51: return 0x10;
    case 52: return 0x14;
    case 53: return 0x19;
    case 54: return 0x1A;
    case 61: return 0x1B;
    case 62: return 0x1F;
    case 63: return 0x17;
    case 64: return 0x12;
    case 71: return 0x15;
    case 72: return 0x1E;
    case 73: return 0x1D;
    case 74: return 0x11;
    case 81: return 0x18;
    case 82: return 0x16;
    case 83: return 0x13;
    case 84: return 0x1C;
    default: panic("Unknown race slot ID: %d", m_slotId);
    }
}

u32 Track::getBattleCourseId() const {
    switch (m_slotId) {
    case 11: return 0x21;
    case 12: return 0x20;
    case 13: return 0x23;
    case 14: return 0x22;
    case 15: return 0x24;
    case 21: return 0x27;
    case 22: return 0x28;
    case 23: return 0x29;
    case 24: return 0x25;
    case 25: return 0x26;
    default: panic("Unknown battle slot ID: %d", m_slotId);
    }
}
// clang-format on

} // namespace SP
