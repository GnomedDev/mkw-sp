#pragma once

#include "game/effect/KartEffect.hh"

namespace Effect {

class EffectManager {
public:
    void REPLACED(createKarts)();
    REPLACE void createKarts();
    void calc();

    static EffectManager *Instance();

private:
    u8 _000[0x068 - 0x000];
    KartEffect **m_karts;
    u8 _06c[0x9f4 - 0x06c];

    static EffectManager *s_instance;
};
static_assert_32bit(sizeof(EffectManager) == 0x9f4);

} // namespace Effect
