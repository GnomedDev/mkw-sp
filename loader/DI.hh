#pragma once

#include <common/IOS.hh>

namespace IOS {

namespace DIResult {
enum {
    Success = 0x1,
    InvalidArgument = 0x80,
};
} // namespace DIResult

class DI final : private Resource {
public:
    DI();
    ~DI() = default;

    bool readDiskID();
    bool readUnencrypted(void *dst, u32 size, u32 offset);
    s32 openPartition(u32 offset);
    bool read(void *dst, u32 size, u32 offset);
    bool isInserted();
    bool reset();
};

} // namespace IOS
