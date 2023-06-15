#pragma once

#include "TrackPack.hh"

namespace SP {

class TrackPackInfo {
public:
    void getTrackPath(char *out, u32 outSize, bool splitScreen) const;

    // May be different to the sha1 passed to selectCourse, due to
    // My Stuff replacements requiring matching Ghost hashes.
    Sha1 getCourseSha1() const;

    u32 getSelectedCourse() const;
    const wchar_t *getCourseName() const;
    std::optional<u32> getSelectedMusic() const;

    void selectCourse(Sha1 id);

private:
    // Private as need to be kept in sync
    u32 m_selectedCourseId = 0;
    u32 m_selectedMusicId;
    Sha1 m_selectedSha1 = {};
    WFixedString<48> m_name;
};

} // namespace SP
