#pragma once

#include <Common.hh>

namespace nw4r::lyt {

class AnimTransform {
public:
    AnimTransform();
    virtual ~AnimTransform();

    u16 getFrameSize() const;

private:
    u8 _04[0x14 - 0x04];
};
static_assert(sizeof(AnimTransform) == 0x14);

} // namespace nw4r::lyt
