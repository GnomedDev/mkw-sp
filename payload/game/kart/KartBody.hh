#pragma once

#include "KartObjectProxy.hh"
#include "KartPart.hh"

namespace Kart {

class KartBody : public KartPart {
    u8 _90[0x234 - 0x90];
};

static_assert_32bit(sizeof(KartBody) == 0x234);

} // namespace Kart
