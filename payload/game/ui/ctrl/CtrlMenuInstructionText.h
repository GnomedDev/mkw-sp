#pragma once

#include "../UIControl.h"

typedef struct {
    LayoutUIControl;
} CtrlMenuInstructionText;
static_assert_32bit(sizeof(CtrlMenuInstructionText) == 0x174);

CtrlMenuInstructionText *CtrlMenuInstructionText_ct(CtrlMenuInstructionText *this);

void CtrlMenuInstructionText_dt(CtrlMenuInstructionText *this, s32 type);

void CtrlMenuInstructionText_load(CtrlMenuInstructionText *this);

void CtrlMenuInstructionText_setMessage(CtrlMenuInstructionText *this, s32 messageId,
        MessageInfo *info);
