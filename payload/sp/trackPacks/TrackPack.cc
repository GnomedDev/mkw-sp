#include "TrackPack.hh"
#include "Parse.hh"

#include <vendor/magic_enum/magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

namespace SP {

std::expected<TrackPack, const char *> TrackPack::New(std::span<const u8> manifestRaw) {
    TrackPack self;
    TRY(self.parseNew(manifestRaw));
    return self;
}

std::expected<void, const char *> TrackPack::parseNew(std::span<const u8> manifestRaw) {
    pb_istream_t stream = pb_istream_from_buffer(manifestRaw.data(), manifestRaw.size());

    Pack manifest = Pack_init_zero;
    manifest.raceTracks.arg = &m_raceTracks;
    manifest.raceTracks.funcs.decode = &decodeSha1Callback;
    manifest.coinTracks.arg = &m_coinTracks;
    manifest.coinTracks.funcs.decode = &decodeSha1Callback;
    manifest.balloonTracks.arg = &m_balloonTracks;
    manifest.balloonTracks.funcs.decode = &decodeSha1Callback;
    manifest.unreleasedTracks.arg = &m_unreleasedTracks;
    manifest.unreleasedTracks.funcs.decode = &decodeTrackCallback;

    if (!pb_decode(&stream, Pack_fields, &manifest)) {
        return std::unexpected("Failed to parse TrackPack");
    }

    auto nameSize = strnlen(manifest.name, sizeof(manifest.name));
    m_prettyName.setUTF8(std::string_view(manifest.name, nameSize));

    m_authorNames = FixedString<64>((const char *)&manifest.authorNames);
    m_description = FixedString<128>((const char *)&manifest.description);

    return {};
}

bool TrackPack::contains(const Sha1 &sha1) const {
    for (auto mode : modes) {
        auto &trackList = getTrackList(mode);
        if (std::find(trackList.begin(), trackList.end(), sha1) != trackList.end()) {
            return true;
        }
    }

    return false;
}

u16 TrackPack::getTrackCount(Track::Mode mode) const {
    return getTrackList(mode).size();
}

Track::Mode TrackPack::getSupportedModes() const {
    auto supportedModes = static_cast<Track::Mode>(0);
    for (auto mode : modes) {
        if (!getTrackList(mode).empty()) {
            supportedModes |= mode;
        }
    }

    return supportedModes;
}

std::optional<Sha1> TrackPack::getNthTrack(u32 n, Track::Mode mode) const {
    auto &trackList = getTrackList(mode);
    if (trackList.size() <= n) {
        return std::nullopt;
    } else {
        return trackList[n];
    }
}

const wchar_t *TrackPack::getPrettyName() const {
    return m_prettyName.c_str();
}

const Track *TrackPack::getUnreleasedTrack(Sha1 sha1) const {
    auto predicate = [&](Track t) { return t.m_sha1 == sha1; };
    auto track = std::find_if(m_unreleasedTracks.begin(), m_unreleasedTracks.end(), predicate);
    return track == m_unreleasedTracks.end() ? nullptr : &*track;
}

const std::vector<Sha1> &TrackPack::getTrackList(Track::Mode mode) const {
    if (mode == Track::Mode::Race) {
        return m_raceTracks;
    } else if (mode == Track::Mode::Balloon) {
        return m_balloonTracks;
    } else if (mode == Track::Mode::Coin) {
        return m_coinTracks;
    } else {
        panic("Unknown track game mode: %d", static_cast<u32>(mode));
    }
}

} // namespace SP
