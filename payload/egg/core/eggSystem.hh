#pragma once

#include <Common.hh>
extern "C" {
#include "egg/core/eggDisplay.h"
}

namespace EGG {

class SceneManager;
class XfbManager;

class TSystem {
public:
    TSystem();
    virtual ~TSystem();
    virtual void *getVideo();
    virtual void *getSysHeap();
    virtual EGGDisplay *getDisplay();
    virtual XfbManager *getXfbManager();
    virtual void *getPerformanceView();
    virtual SceneManager *getSceneManager();
    virtual void *getAudioManager();
    virtual void onBeginFrame();
    virtual void onEndFrame();
    virtual void initRenderMode();
    virtual void initMemory();
    virtual void run();
    virtual void initialize();

    void *mem1ArenaLo() const;
    void *mem1ArenaHi() const;
    void *mem2ArenaLo() const;
    void *mem2ArenaHi() const;

    static TSystem *Instance();

private:
    u8 _00[0x04 - 0x00];
    void *m_mem1ArenaLo;
    void *m_mem1ArenaHi;
    void *m_mem2ArenaLo;
    void *m_mem2ArenaHi;
    // ...

    static TSystem s_instance;
};

} // namespace EGG
