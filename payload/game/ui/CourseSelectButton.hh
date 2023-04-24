#pragma once

#include "Button.hh"

namespace UI {

class CourseSelectButton : public PushButton {
public:
    CourseSelectButton();
    ~CourseSelectButton();

    void load(u32 i);
    void refresh(u32 messageId);
    void refresh(u8 c, const GXTexObj &texObj);

    bool m_isRandom;
};

} // namespace UI
