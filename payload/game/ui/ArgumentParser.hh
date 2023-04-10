#pragma once

#include "game/ui/SectionId.hh"

#include <Common.hh>

namespace UI {

class ArgumentParser {
public:
    REPLACE SectionId parse();
};
static_assert_32bit(sizeof(ArgumentParser) == 0x1);

} // namespace UI
