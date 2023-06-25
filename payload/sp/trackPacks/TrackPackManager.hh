#pragma once

#include "TrackPack.hh"

namespace SP {

class TrackPackManager {
public:
    TrackPackManager();
    TrackPackManager(const TrackPackManager &) = delete;

private:
    std::expected<void, const char *> loadTrackPacks();
    void loadTrackMetadata();
    void loadTrack(std::span<u8> manifestBuf, Sha1 sha1);

public:
    size_t getPackCount() const;
    const Track &getTrack(Sha1 id) const;

    const TrackPack &getNthPack(u32 n) const;
    const TrackPack &getSelectedTrackPack() const;

    static TrackPackManager &Instance();
    static void CreateInstance();
    static void DestroyInstance();

private:
    bool anyPackContains(const Sha1 &sha1);

    std::vector<Track> m_trackDb;
    std::vector<TrackPack> m_packs;

    static TrackPackManager *s_instance;
};

} // namespace SP
