#pragma once

#include "egg/core/eggHeap.hh"
#include "egg/core/eggXfbManager.hh"

namespace EGG {

class TSystem {
public:
    TSystem();
    virtual ~TSystem();
    virtual void dt(s32 type);
    virtual void *getVideo();
    virtual void *getSysHeap();
    virtual void *getDisplay();
    virtual XfbManager *getXfbManager();
    virtual void *getPerformanceView();
    virtual void *getSceneManager();
    virtual void *getAudioManager();
    REPLACE virtual void onBeginFrame();
    REPLACE virtual void onEndFrame();
    virtual void initRenderMode();
    virtual void initMemory();
    virtual void run();
    virtual void initialize();

    void *mem1ArenaLo() const;
    void *mem1ArenaHi() const;
    void *mem2ArenaLo() const;
    void *mem2ArenaHi() const;
    Heap *eggRootMEM1() const;
    Heap *eggRootMEM2() const;
    Heap *eggRootDebug() const;
    Heap *eggRootSystem() const;

    static TSystem &Instance();

private:
    void *m_mem1ArenaLo;
    void *m_mem1ArenaHi;
    void *m_mem2ArenaLo;
    void *m_mem2ArenaHi;
    u8 _14[0x18 - 0x14];
    Heap *m_eggRootMEM1;
    Heap *m_eggRootMEM2;
    Heap *m_eggRootDebug;
    Heap *m_eggRootSystem;
    u8 _28[0x6d - 0x28];

protected:
    u8 m_consoleInputUnavailable;

private:
    u8 _6e[0x74 - 0x6e];

    static TSystem s_instance;
};

static_assert(sizeof(TSystem) == 0x74);

} // namespace EGG
