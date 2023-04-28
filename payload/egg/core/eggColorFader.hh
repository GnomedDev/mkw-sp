#pragma once

namespace EGG {

class ColorFader {
public:
    ColorFader();
    virtual ~ColorFader();

    bool fadeIn();
    bool fadeOut();

private:
    char _00[0x24 - 0x00];
};

}; // namespace EGG
