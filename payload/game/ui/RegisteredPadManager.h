#pragma once

#include <Common.h>

enum {
    REGISTERED_PAD_TYPE_GC = 0x4,
};

#define REGISTERED_PAD_FLAGS_GET_TYPE(flags) ((flags) >> 0 & 0xf)

typedef struct {
    u8 _00[0x5c - 0x00];
} RegisteredPadManager;
static_assert_32bit(sizeof(RegisteredPadManager) == 0x5c);

u32 RegisteredPadManager_getFlags(RegisteredPadManager *this, u32 localPlayerId);
