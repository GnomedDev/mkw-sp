#include "RkSystem.hh"

#include <sp/MapFile.hh>

namespace System {

void RkSystem::initialize() {
    REPLACED(initialize)();

    m_consoleInputUnavailable = 0;
    SP::MapFile::Load();
}

} // namespace System
