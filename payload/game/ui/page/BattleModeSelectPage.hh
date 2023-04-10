#pragma once

#include "game/ui/page/MenuPage.hh"

namespace UI {

class BattleModeSelectPage : public MenuPage {
private:
    BattleModeSelectPage();
    ~BattleModeSelectPage() override;

    void REPLACED(onInit)();
    REPLACE void onInit() override;
    REPLACE void onButtonFront(const PushButton *button);
    u8 _0[0x6c4 - 0x430];
};
static_assert_32bit(sizeof(BattleModeSelectPage) == 0x6c4);

} // namespace UI
