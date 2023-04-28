#pragma once

#include "nw4r/lyt/lyt_pane.hh"

namespace nw4r::lyt {

class TextBox : public Pane {
public:
    virtual void vf_74();
    virtual void freeStringBuffer();
    virtual void setString(const wchar_t *str, u16 outPosition);
    virtual void setString2();

private:
    u8 _0d8[0x104 - 0x0d8];
};
static_assert(sizeof(TextBox) == 0x104);

} // namespace nw4r::lyt
