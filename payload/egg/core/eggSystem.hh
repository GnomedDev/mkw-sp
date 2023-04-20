#pragma once

#include "egg/core/eggXfbManager.hh"

namespace EGG {

class TSystem {
public:
    TSystem();
    virtual ~TSystem();
    virtual void *dt(s32 type);

    virtual void *getVideo();
    virtual void *getSysHeap();
    virtual void *getDisplay();
    virtual XfbManager* getXfbManager();
    virtual void *getPerformanceView();
    virtual void *getSceneManager();
    virtual void *getAudioManager();
    REPLACE virtual void onBeginFrame();

    void *mem1ArenaLo() const;
    void *mem1ArenaHi() const;
    void *mem2ArenaLo() const;
    void *mem2ArenaHi() const;

    static TSystem *Instance();

private:
    void *m_mem1ArenaLo;
    void *m_mem1ArenaHi;
    void *m_mem2ArenaLo;
    void *m_mem2ArenaHi;
    u8 _14[0x48 - 0x14];
    XfbManager *m_xfbManager;
    u8 _4c[0x68 - 0x4c];
    u8 m_frameClock;
    u8 m_consoleInputsUnavailable; // Was padding
    u8 _6a[0x74 - 0x6a];

    static TSystem s_instance;
};

static_assert(sizeof(TSystem) == 0x74);

} // namespace EGG
