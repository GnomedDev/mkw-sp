#include "KartObject.hh"

#include <sp/cs/RoomClient.hh>

namespace Kart {

void KartObject::init() {
    REPLACED(init)();
    m_accessor.rollback = nullptr;
}

void KartObject::calcEarly() {
    m_sub->calcEarly();
}

void KartObject::calcLate() {
    REPLACED(calcLate)();
}

} // namespace Kart
