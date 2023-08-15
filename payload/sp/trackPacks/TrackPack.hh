#pragma once

#include "Track.hh"

#include <vendor/nanopb/pb.h>

#include <expected>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace SP {

class TrackPack {
public:
    static std::expected<TrackPack, const char *> New(std::span<const u8> manifest);

private:
    std::expected<void, const char *> parseNew(std::span<const u8> manifest);

public:
    const Sha1 &getManifestSha() const;
    Track::Mode getSupportedModes() const;
    u16 getTrackCount(Track::Mode mode) const;
    std::optional<Sha1> getNthTrack(u32 n, Track::Mode mode) const;
    void forEachTrack(std::function<void(const Sha1 &)> callback) const;

    const wchar_t *getPrettyName() const;

private:
    TrackPack() = default;

    const std::vector<Sha1> &getTrackList(Track::Mode mode) const;

    Sha1 m_manifestSha;

    std::vector<Sha1> m_raceTracks;
    std::vector<Sha1> m_coinTracks;
    std::vector<Sha1> m_balloonTracks;

    FixedString<64> m_authorNames;
    FixedString<128> m_description;
    WFixedString<64> m_prettyName;
};

} // namespace SP
