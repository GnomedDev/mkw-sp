#pragma once

#include <Common.hh>

namespace Obj {

class Object {
protected:
    Object();
    virtual ~Object();

    virtual void vf_0c();
    virtual void vf_10();
    virtual void vf_14();
    virtual void vf_18();
    virtual void vf_1c();
    virtual void init();
    virtual u16 getId();
    virtual char *getName();
    virtual void vf_2c();
    virtual void vf_30();
    virtual void vf_34();
    virtual void vf_38();
    virtual void vf_3c();
    virtual void vf_40();
    virtual void vf_44();
    virtual void vf_48();
    virtual void vf_4c();
    virtual void vf_50();
    virtual void vf_54();
    virtual void vf_58();
    virtual void vf_5c();
    virtual void vf_60();
    virtual void vf_64();
    virtual void vf_68();
    virtual void vf_6c();
    virtual void vf_70();
    virtual void vf_74();
    virtual void vf_78();
    virtual void vf_7c();
    virtual void vf_80();
    virtual void vf_84();
    virtual void vf_88();
    virtual void vf_8c();
    virtual void vf_90();
    virtual void vf_94();
    virtual void vf_98();
    virtual void vf_9c();
    virtual void vf_a0();
    virtual void vf_a4();
    virtual void vf_a8();
    virtual void vf_ac();
    virtual u32 vf_b0();

private:
    u8 _04[0xb0 - 0x04];
};

static_assert(sizeof(Object) == 0xb0);

} // namespace Obj