#pragma once

extern "C" {
#include "lyt_Rect.h"
}

namespace nw4r::lyt {

class DrawInfo {
public:
    DrawInfo();
    virtual ~DrawInfo();
    virtual void dt(s32 type);

    f32 viewMtx[3][4];
    Rect viewRect;

private:
    Vec2<f32> locationAdjustScale;
    u8 _4c[0x50 - 0x4c];
    u8 _50_0 : 2;
    bool locationAdjust : 1;
    u8 _50_4 : 5;
};

static_assert(sizeof(DrawInfo) == 0x54);

} // namespace nw4r::lyt
