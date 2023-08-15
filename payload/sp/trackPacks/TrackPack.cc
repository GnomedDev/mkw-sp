#include "TrackPack.hh"

#include "sp/trackPacks/Parse.hh"

#include <protobuf/TrackPacks.pb.h>
#include <vendor/magic_enum/magic_enum.hpp>
extern "C" {
#include <revolution/net.h>
}

using namespace magic_enum::bitwise_operators;

namespace SP {

std::expected<TrackPack, const char *> TrackPack::New(std::span<const u8> manifestRaw) {
    TrackPack self;
    TRY(self.parseNew(manifestRaw));
    return self;
}

std::expected<void, const char *> TrackPack::parseNew(std::span<const u8> manifestRaw) {
    NETSHA1Context context;
    NETSHA1Init(&context);
    NETSHA1Update(&context, manifestRaw.data(), manifestRaw.size());
    NETSHA1GetDigest(&context, &m_manifestSha);

    pb_istream_t stream = pb_istream_from_buffer(manifestRaw.data(), manifestRaw.size());

    Pack manifest = Pack_init_zero;
    manifest.raceTracks.arg = &m_raceTracks;
    manifest.raceTracks.funcs.decode = &decodeSha1Callback;
    manifest.coinTracks.arg = &m_coinTracks;
    manifest.coinTracks.funcs.decode = &decodeSha1Callback;
    manifest.balloonTracks.arg = &m_balloonTracks;
    manifest.balloonTracks.funcs.decode = &decodeSha1Callback;

    if (!pb_decode(&stream, Pack_fields, &manifest)) {
        return std::unexpected("Failed to parse TrackPack");
    }

    auto nameSize = strnlen(manifest.name, sizeof(manifest.name));
    m_prettyName.setUTF8(std::string_view(manifest.name, nameSize));

    m_authorNames = FixedString<64>(manifest.authorNames);
    m_description = FixedString<128>(manifest.description);

    return {};
}

const Sha1 &TrackPack::getManifestSha() const {
    return m_manifestSha;
}

u16 TrackPack::getTrackCount(Track::Mode mode) const {
    return getTrackList(mode).size();
}

Track::Mode TrackPack::getSupportedModes() const {
    auto supportedModes = static_cast<Track::Mode>(0);
    for (auto mode : s_trackModes) {
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

void TrackPack::forEachTrack(std::function<void(const Sha1 &)> callback) const {
    for (auto mode : s_trackModes) {
        std::optional<Sha1> trackSha;
        for (u16 i = 0; (trackSha = getNthTrack(i, mode)); i += 1) {
            callback(*trackSha);
        }
    }
}

const wchar_t *TrackPack::getPrettyName() const {
    return m_prettyName.c_str();
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
