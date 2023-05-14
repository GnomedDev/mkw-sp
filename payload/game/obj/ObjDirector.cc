#include "ObjDirector.hh"

#include <sp/3d/Checkpoints.hh>

namespace Geo {

void ObjDirector::drawDebug() {
    SP::DrawCheckpoints();
    REPLACED(drawDebug)();
}

ObjDirector *ObjDirector::Instance() {
    return s_instance;
}

} // namespace Geo
