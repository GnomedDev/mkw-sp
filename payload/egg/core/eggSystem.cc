#include "eggSystem.hh"

extern "C" {
#include <game/system/Console.h>
#include <sp/Commands.h>
#include <sp/ItemCommand.h>
#include <sp/keyboard/Keyboard.h>
}

namespace EGG {

void *TSystem::mem1ArenaLo() const {
    return m_mem1ArenaLo;
}

void *TSystem::mem1ArenaHi() const {
    return m_mem1ArenaHi;
}

void *TSystem::mem2ArenaLo() const {
    return m_mem2ArenaLo;
}

void *TSystem::mem2ArenaHi() const {
    return m_mem2ArenaHi;
}

Heap *TSystem::eggRootMEM1() const {
    return m_eggRootMEM1;
}

Heap *TSystem::eggRootMEM2() const {
    return m_eggRootMEM2;
}

Heap *TSystem::eggRootDebug() const {
    return m_eggRootDebug;
}

Heap *TSystem::eggRootSystem() const {
    return m_eggRootSystem;
}

TSystem &TSystem::Instance() {
    return s_instance;
}

void TSystem::onBeginFrame() {
    Item_beginFrame();
    if (m_consoleInputUnavailable) {
        return;
    }

    if (!SP_IsConsoleInputInit()) {
        if (SP_InitConsoleInput()) {
            Commands_init();
            SP_SetLineCallback(Commands_lineCallback);
        } else {
            // Do not try again
            m_consoleInputUnavailable = true;
            return;
        }
    }

    SP_ProcessConsoleInput();
    Console_calc();
}

} // namespace EGG
