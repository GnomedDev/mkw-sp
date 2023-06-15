#include "ThumbnailManager.hh"

#include "sp/trackPacks/TrackPackManager.hh"

#include <egg/core/eggSystem.hh>
#include <game/system/RaceConfig.hh>

namespace SP {

void ThumbnailManager::Start() {
    s_instance.emplace();
}

bool ThumbnailManager::Continue() {
    assert(s_instance);
    s_instance->capture();

    return Next();
}

bool ThumbnailManager::IsActive() {
    return s_instance.has_value();
}

bool ThumbnailManager::next() {
    auto *raceConfig = System::RaceConfig::Instance();
    if (m_hasStarted) {
        raceConfig->nextCourseIndex();
    } else {
        m_hasStarted = true;
    }

    auto &trackPackManager = TrackPackManager::Instance();
    auto &trackPack = trackPackManager.getSelectedTrackPack();
    auto trackId = trackPack.getNthTrack(raceConfig->getCourseIndex(), Track::Mode::Race);

    if (trackId.has_value()) {
        raceConfig->emplacePackInfo().selectCourse(*trackId);
        return true;
    } else {
        return false;
    }
}

void ThumbnailManager::capture() {
    std::array<wchar_t, 256> path{};

    swprintf(path.data(), path.size(), L"/mkw-sp/Generated Thumbnails");
    if (!Storage::CreateDir(path.data(), true)) {
        return;
    }

    auto sha1 = System::RaceConfig::Instance()->getPackInfo().getCourseSha1();
    auto hex = sha1ToHex(sha1);

    swprintf(path.data(), path.size(), L"/mkw-sp/Generated Thumbnails/%s.xfb", hex.data());

    auto *xfb = EGG::TSystem::Instance().xfbManager()->headXfb();
    u32 size = EGG::Xfb::CalcXfbSize(xfb->width(), xfb->height());
    Storage::WriteFile(path.data(), xfb->buffer(), size, true);
}

bool ThumbnailManager::Next() {
    if (s_instance && !s_instance->next()) {
        s_instance.reset();
    }

    return IsActive();
}

std::optional<ThumbnailManager> ThumbnailManager::s_instance{};

} // namespace SP
