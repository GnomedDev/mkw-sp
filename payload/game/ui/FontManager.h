#pragma once

#include "Font.h"

extern Font *sDebugFont;

typedef struct {
    Font *fonts[6];
} FontManager;
static_assert_32bit(sizeof(FontManager) == 0x18);

void FontManager_init(FontManager *this);
