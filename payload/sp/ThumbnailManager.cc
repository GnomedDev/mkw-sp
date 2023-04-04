#include "ThumbnailManager.hh"

extern "C" {
#include "sp/WideUtil.h"
}
#include "sp/TrackPackManager.hh"

#include <egg/core/eggSystem.hh>
#include <game/system/RaceConfig.hh>

#include <cwchar>
#include <iterator>

namespace SP {

bool ThumbnailManager::Start() {
    auto *xfb = EGG::TSystem::Instance()->xfbManager()->headXfb();
    u32 size = EGG::Xfb::CalcXfbSize(xfb->width(), xfb->height());

    u16 *buffer = static_cast<u16 *>(xfb->buffer());
    for (u32 i = 0; i < size / 2; i += 1) {
        if (buffer[i] != 0x01FE) {
            s_instance.emplace();
            return true;
        }
    }

    return false;
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
    auto &trackPackManager = TrackPackManager::Instance();
    auto &trackPack = trackPackManager.getSelectedTrackPack();
    auto wiimmId = trackPack.getNthTrack(m_trackIndex++, TrackGameMode::Race);

    if (wiimmId.has_value()) {
        auto *raceConfig = System::RaceConfig::Instance();
        raceConfig->m_packInfo.selectCourse(*wiimmId);
        return true;
    } else {
        return false;
    }
}

void ThumbnailManager::capture() {
    std::array<wchar_t, 64> path{};

    swprintf(path.data(), path.size(), L"/mkw-sp/Generated Thumbnails");
    if (!Storage::CreateDir(path.data(), true)) {
        return;
    }

    auto wiimmId = System::RaceConfig::Instance()->m_packInfo.getSelectedWiimmId();
    swprintf(path.data(), path.size(), L"/mkw-sp/Generated Thumbnails/%u.xfb", wiimmId);

    auto *xfb = EGG::TSystem::Instance()->xfbManager()->headXfb();
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
