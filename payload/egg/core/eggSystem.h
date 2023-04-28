#pragma once

#include <egg/core/eggDisplay.h>
#include <egg/core/eggScene.h>
#include <egg/core/eggSceneManager.h>
#include <revolution.h>

typedef struct {
    GXRenderModeObj *renderMode;
} EGGVideo;

typedef struct {
    char _[0x44 - 0x00];
    EGGVideo *video;
    void *xfbMgr;
    EGGDisplay *display;
    void *perfView;
    EGG_SceneManager *scnMgr;
    // ...
} EGGTSystem;

extern EGGTSystem sRKSystem;

void EGG_ConfigurationData_onBeginFrame(void *system);
void EGG_ProcessMeter_draw(void *);
