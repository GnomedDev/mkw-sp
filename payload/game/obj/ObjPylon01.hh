#pragma once

#include "Obj.hh"

namespace Obj {

class Pylon01 : public Object {
public:
    Pylon01();
    virtual ~Pylon01();

private:
    REPLACE virtual u32 vf_b0() override;

    u8 _0b0[0x108 - 0x0b0];
    u32 m_playerId;
    u8 _10c[0x118 - 0x10c];
};

static_assert(sizeof(Pylon01) == 0x118);

} // namespace Obj
