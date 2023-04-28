#pragma once

#include "egg/core/eggDisposer.hh"

namespace EGG {
class Heap;
class SceneManager;

class Scene : public Disposer {
public:
    Scene();
    virtual ~Scene();
    virtual void dt(s32 type);
    virtual void calc();
    virtual void draw();
    virtual void enter();
    virtual void exit();
    virtual void reinit();
    virtual void incoming_childDestroy();
    virtual void outgoing_childCreate();

    Scene *getParent() const {
        return m_parent;
    }

    Scene *getChild() const {
        return m_child;
    }

    u32 getSceneID() const {
        return m_sceneID;
    }

public:
    Heap *m_heapParent;
    Heap *m_heapMem1;
    Heap *m_heapMem2;
    Heap *m_heapDebug;

protected:
    Scene *m_parent;
    Scene *m_child;
    u32 m_sceneID;
    SceneManager *m_sceneManager;
};
static_assert(sizeof(Scene) == 0x30);

} // namespace EGG
