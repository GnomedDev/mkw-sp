#pragma once

#include "KartObjectProxy.h"

typedef struct {
    KartObjectProxy;
    u8 _0c[0x80 - 0x0c];
} KartObject;
static_assert_32bit(sizeof(KartObject) == 0x80);
