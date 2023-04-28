#pragma once

#include "lyt_arcResourceLink.hh"

namespace nw4r::lyt {

class MultiArcResourceAccessor {
public:
    MultiArcResourceAccessor();
    virtual ~MultiArcResourceAccessor();
    virtual void dt(s32 type);

    void attach(ArcResourceLink &link);
    void *getResource(u32 type, const char *path, u32 *len) const;

private:
    u8 _04[0x1c - 0x04];
};

static_assert(sizeof(MultiArcResourceAccessor) == 0x1c);

} // namespace nw4r::lyt
