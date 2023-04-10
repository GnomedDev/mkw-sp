#pragma once

#include <Common.hh>

typedef void (*GameEntryFunc)(s8 idx);

namespace UI {

class DriverModel {
public:
    void setAnim(u32 r4, u32 r5);

private:
    u8 _00[0x28 - 0x00];
};
static_assert_32bit(sizeof(DriverModel) == 0x28);

struct DriverModelHandle {
    u8 _0[0x8 - 0x0];
    DriverModel *model;
};
static_assert_32bit(sizeof(DriverModelHandle) == 0xc);

class DriverModelManager {
public:
    f32 getDelay() const;
    DriverModelHandle *handle(size_t index);

private:
    u8 _00[0x0c - 0x00];
    s32 m_delay;
    DriverModelHandle m_handles[4];
    u8 _40[0x54 - 0x40];
};
static_assert_32bit(sizeof(DriverModelManager) == 0x54);

class MenuModelManager {
public:
    DriverModelManager *driverModelManager();

    void init(u32 modelCount, GameEntryFunc onDriverModelLoaded);

    static MenuModelManager *Instance();

private:
    u8 _00[0x14 - 0x00];
    DriverModelManager *m_driverModelManager;
    u8 _18[0x40 - 0x18];

    static MenuModelManager *s_instance;
};
static_assert_32bit(sizeof(MenuModelManager) == 0x40);

} // namespace UI
