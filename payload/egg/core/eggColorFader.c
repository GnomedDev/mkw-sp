#include <revolution.h>
#include <sp/Host.h>

bool EGG_ColorFader_fadeIn(void *colorFader);
bool EGG_ColorFader_fadeOut(void *colorFader);

static bool post_fadeIn(bool changed) {
    if (HostPlatform_IsDolphin(Host_GetPlatform()) && changed) {
        axIsMuted = false;
    }

    return changed;
}

static bool post_fadeOut(bool changed) {
    if (HostPlatform_IsDolphin(Host_GetPlatform()) && changed) {
        axIsMuted = true;
    }

    return changed;
}

PATCH_B(EGG_ColorFader_fadeIn + 0x24, post_fadeIn);
PATCH_B(EGG_ColorFader_fadeOut + 0x28, post_fadeOut);
