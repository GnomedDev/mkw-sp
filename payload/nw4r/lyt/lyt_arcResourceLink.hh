#pragma once

#include <Common.hh>

namespace nw4r::lyt {

class ArcResourceLink {
public:
    void set(void *buffer, const char *root);

private:
    // Size is tentative (based on array load in game code)
    u8 _[0xa4 - 0x00];
};

} // namespace nw4r::lyt
