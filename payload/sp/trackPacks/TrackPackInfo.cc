#include "TrackPackInfo.hh"
#include "TrackPackManager.hh"

#include <game/system/RaceConfig.hh>
#include <game/system/SaveManager.hh>
#include <game/util/Registry.hh>
extern "C" {
#include <revolution.h>
}

#include <cstring>

namespace SP {

void TrackPackInfo::getTrackPath(char *out, u32 outSize, bool splitScreen) const {
    auto hex = sha1ToHex(m_selectedSha1);
    SP_LOG("Getting track path for %s", hex.data());

    if (System::RaceConfig::Instance()->isVanillaTracks()) {
        auto courseFileName = Registry::courseFilenames[m_selectedCourseId];

        if (splitScreen) {
            snprintf(out, outSize, "Race/Course/%s_d", courseFileName);
        } else {
            snprintf(out, outSize, "Race/Course/%s", courseFileName);
        }

        SP_LOG("Vanilla Track path: %s", out);
    } else {
        snprintf(out, outSize, "/mkw-sp/Tracks/%s", hex.data());
        SP_LOG("Track path: %s", out);
    }
}

Sha1 TrackPackInfo::getCourseSha1() const {
    if (System::RaceConfig::Instance()->isVanillaTracks()) {
        auto *saveManager = System::SaveManager::Instance();
        auto myStuffSha1 = saveManager->courseSHA1(m_selectedCourseId);
        if (myStuffSha1.has_value()) {
            return *myStuffSha1;
        }
    }

    return m_selectedSha1;
}

u32 TrackPackInfo::getSelectedCourse() const {
    return m_selectedCourseId;
}

const wchar_t *TrackPackInfo::getCourseName() const {
    return m_name.c_str();
}

std::optional<u32> TrackPackInfo::getSelectedMusic() const {
    if (m_selectedMusicId == std::numeric_limits<u32>::max()) {
        return std::nullopt;
    } else {
        return m_selectedMusicId;
    }
}

void TrackPackInfo::selectCourse(Sha1 sha) {
    auto &track = TrackPackManager::Instance().getTrack(sha);

    m_name = track.m_name;
    m_selectedSha1 = track.m_sha1;
    m_selectedMusicId = track.m_musicId;
    m_selectedCourseId = track.getCourseId();
}

} // namespace SP
