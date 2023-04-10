#pragma once

#include "nw4r/snd/SoundArchive.hh"

namespace nw4r::snd {

class SoundArchiveLoader {
public:
    SoundArchiveLoader(SoundArchive *archive);
    ~SoundArchiveLoader();

private:
    u8 _000[0x220 - 0x000];
};
static_assert_32bit(sizeof(SoundArchiveLoader) == 0x220);

} // namespace nw4r::snd
