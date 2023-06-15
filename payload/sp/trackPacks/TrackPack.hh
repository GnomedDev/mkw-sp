#pragma once

#include "Track.hh"

#include <expected>
#include <span>
#include <vector>

namespace SP {

class TrackPack {
public:
    static std::expected<TrackPack, const char *> New(std::span<const u8> manifest);

private:
    std::expected<void, const char *> parseNew(std::span<const u8> manifest);

public:
    bool contains(const Sha1 &id) const;
    Track::Mode getSupportedModes() const;
    u16 getTrackCount(Track::Mode mode) const;
    std::optional<Sha1> getNthTrack(u32 n, Track::Mode mode) const;

    const wchar_t *getPrettyName() const;
    const Track *getUnreleasedTrack(Sha1 id) const;

private:
    TrackPack() = default;

    const std::vector<Sha1> &getTrackList(Track::Mode mode) const;

    std::vector<Sha1> m_raceTracks;
    std::vector<Sha1> m_coinTracks;
    std::vector<Sha1> m_balloonTracks;
    std::vector<Track> m_unreleasedTracks;

    FixedString<64> m_authorNames;
    FixedString<128> m_description;
    WFixedString<64> m_prettyName;
};

} // namespace SP
