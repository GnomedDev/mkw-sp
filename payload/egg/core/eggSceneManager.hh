#pragma once

#include "egg/core/eggScene.hh"

namespace EGG {
class ColorFader;

class SceneManager {
public:
    void REPLACED(reinitCurrentScene)();
    void REPLACE reinitCurrentScene();

    void REPLACED(createScene)(s32 sceneId, Scene *parent);
    void REPLACE createScene(s32 sceneId, Scene *parent);

    void REPLACED(destroyScene)(Scene *scene);
    void REPLACE destroyScene(Scene *scene);

private:
    static bool InitDolphinSpeed();
    static bool SetDolphinSpeed(u32 percent);
    static u32 GetDolphinSpeedLimit();
    static void PushDolphinSpeed(u32 percent);
    static void PopDolphinSpeed();

    u8 _00[0x0c - 0x00];

public:
    Scene *m_currScene;

private:
    u8 _10[0x24 - 0x10];

public:
    ColorFader *fader;

private:
    u8 _28[0x2c - 0x28];

    static bool s_dolphinIsUnavailable;
    static u32 s_dolphinSpeedStack[8];
    static s32 s_dolphinSpeedStackSize;
};
static_assert(sizeof(SceneManager) == 0x2c);

} // namespace EGG
