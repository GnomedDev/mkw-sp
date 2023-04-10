#pragma once

#include <Common.h>

typedef void (*TFunction)(void *);

typedef struct {
    u8 _00[0x58 - 0x00];
} EGG_TaskThread;
static_assert_32bit(sizeof(EGG_TaskThread) == 0x58);

void EGG_TaskThread_request(EGG_TaskThread *self, TFunction mainFunction, void *arg,
        void *taskEndMessage);
