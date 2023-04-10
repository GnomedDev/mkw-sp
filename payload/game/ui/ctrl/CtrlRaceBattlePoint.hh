#pragma once

#include "game/ui/ctrl/CtrlRaceBase.hh"

namespace UI {

class CtrlRaceBattlePoint : public CtrlRaceBase {
public:
    CtrlRaceBattlePoint();
    ~CtrlRaceBattlePoint() override;

    REPLACE void init() override;
    REPLACE void calcSelf() override;
    void vf_20() override;
    void vf_28() override;
    void vf_2c() override;
    void vf_40() override;

    void load(const char *variant, u32 screenId);

private:
    void refresh(u32 score);

    u32 m_score;
};
static_assert_32bit(sizeof(CtrlRaceBattlePoint) == 0x19c);

} // namespace UI
