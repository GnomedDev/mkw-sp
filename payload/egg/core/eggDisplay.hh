#pragma once

#include <Common.hh>

namespace EGG {

struct Padding {
    u32 flag;
};

class Display : Padding {
protected:
    virtual void dummy_00();
    virtual void dummy_04();

public:
    virtual void beginFrame();
    virtual void beginRender();
    virtual void endRender();
    REPLACE virtual void endFrame();

private:
    u8 _04[0x28 - 0x08];
};
static_assert(sizeof(Display) == 0x28);

} // namespace EGG
