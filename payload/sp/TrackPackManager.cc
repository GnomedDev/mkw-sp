#include "TrackPackManager.hh"

#include "sp/ThumbnailManager.hh"
#include "sp/settings/IniReader.hh"
#include "sp/storage/Storage.hh"

#include <game/system/RaceConfig.hh>
#include <game/system/ResourceManager.hh>
#include <game/system/SaveManager.hh>
#include <game/ui/UIControl.hh>
#include <game/util/Registry.hh>
#include <vendor/magic_enum/magic_enum.hpp>

#include <protobuf/TrackPacks.pb.h>
#include <vendor/nanopb/pb_decode.h>
#include <vendor/nanopb/pb_encode.h>

#include <charconv>
#include <cstring>

#define TRACK_PACK_DIRECTORY L"Track Packs"
#define TRACK_DB L"TrackDB.pb.bin"

using namespace magic_enum::bitwise_operators;

namespace SP {

bool decodeSha1Callback(pb_istream_t *stream, const pb_field_t * /* field */, void **arg) {
    auto &out = *reinterpret_cast<std::vector<Sha1> *>(*arg);

    ProtoSha1 sha1;
    if (!pb_decode(stream, ProtoSha1_fields, &sha1)) {
        panic("Failed to decode Sha1: %s", PB_GET_ERROR(stream));
    }

    assert(sha1.data.size == 0x14);

    out.push_back(std::to_array(sha1.data.bytes));
    return true;
}

bool decodeTrackCallback(pb_istream_t *stream, const pb_field_t * /* field */, void **arg) {
    auto &out = *reinterpret_cast<std::vector<Track> *>(*arg);

    ProtoTrack protoTrack;
    if (!pb_decode(stream, ProtoTrack_fields, &protoTrack)) {
        panic("Failed to decode track: %s", PB_GET_ERROR(stream));
    }

    assert(protoTrack.sha1.data.size == 0x14);
    Sha1 sha1 = std::to_array(protoTrack.sha1.data.bytes);

    out.emplace_back(sha1, protoTrack.slotId, protoTrack.type == 2, (const char *)&protoTrack.name);

    if (protoTrack.has_musicId) {
        out.back().m_musicId = protoTrack.musicId;
    }

    return true;
}

bool TrackPackManager::DecodeAliasCallback(pb_istream_t *stream, const pb_field_t * /* field */, void **arg) {
    auto *self = reinterpret_cast<TrackPackManager* >(*arg);
    return self->decodeAliasCallback(stream);
}

bool TrackPackManager::decodeAliasCallback(pb_istream_t *stream) {
    TrackDB_AliasValue protoAlias;
    if (!pb_decode(stream, TrackDB_AliasValue_fields, &protoAlias)) {
        panic("Failed to decode alias: %s", PB_GET_ERROR(stream));
    }

    assert(protoAlias.aliased.data.size == 0x14);
    Sha1 aliased = std::to_array(protoAlias.aliased.data.bytes);

    assert(protoAlias.real.data.size == 0x14);
    Sha1 real = std::to_array(protoAlias.real.data.bytes);

    m_aliases.emplace_back(aliased, real);
    return true;
}

// clang-format off
u32 Track::getRaceCourseId() const {
    switch (m_slotId) {
    case 11: return 0x8;
    case 12: return 0x1;
    case 13: return 0x2;
    case 14: return 0x4;
    case 21: return 0x0;
    case 22: return 0x5;
    case 23: return 0x6;
    case 24: return 0x7;
    case 31: return 0x9;
    case 32: return 0xF;
    case 33: return 0xB;
    case 34: return 0x3;
    case 41: return 0xE;
    case 42: return 0xA;
    case 43: return 0xC;
    case 44: return 0xD;
    case 51: return 0x10;
    case 52: return 0x14;
    case 53: return 0x19;
    case 54: return 0x1A;
    case 61: return 0x1B;
    case 62: return 0x1F;
    case 63: return 0x17;
    case 64: return 0x12;
    case 71: return 0x15;
    case 72: return 0x1E;
    case 73: return 0x1D;
    case 74: return 0x11;
    case 81: return 0x18;
    case 82: return 0x16;
    case 83: return 0x13;
    case 84: return 0x1C;
    default: panic("Unknown race slot ID: %d", m_slotId);
    }
}

u32 Track::getBattleCourseId() const {
    switch (m_slotId) {
    case 11: return 0x21;
    case 12: return 0x20;
    case 13: return 0x23;
    case 14: return 0x22;
    case 15: return 0x24;
    case 21: return 0x27;
    case 22: return 0x28;
    case 23: return 0x29;
    case 24: return 0x25;
    case 25: return 0x26;
    default: panic("Unknown battle slot ID: %d", m_slotId);
    }
}
// clang-format on

Track::Track(Sha1 sha1, u8 slotId, bool isArena, const char *name) {
    m_sha1 = sha1;
    m_slotId = slotId;
    m_isArena = isArena;

    auto nameSize = strnlen(name, 0x48);
    m_name.setUTF8(std::string_view(name, nameSize));
}

u32 Track::getCourseId() const {
    if (m_isArena) {
        return getBattleCourseId();
    } else {
        return getRaceCourseId();
    }
}

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

const Track *TrackPack::getUnreleasedTrack(Sha1 sha1) const {
    auto predicate = [&](Track t) { return t.m_sha1 == sha1; };
    auto track = std::find_if(m_unreleasedTracks.begin(), m_unreleasedTracks.end(), predicate);
    return track == m_unreleasedTracks.end() ? nullptr : &*track;
}

const std::vector<Sha1> &TrackPack::getTrackList(TrackGameMode mode) const {
    if (mode == TrackGameMode::Race) {
        return m_raceTracks;
    } else if (mode == TrackGameMode::Balloon) {
        return m_balloonTracks;
    } else if (mode == TrackGameMode::Coin) {
        return m_coinTracks;
    } else {
        panic("Unknown track game mode: %d", static_cast<u32>(mode));
    }
}

constexpr TrackGameMode modes[] = {
        TrackGameMode::Race,
        TrackGameMode::Coin,
        TrackGameMode::Balloon,
};

TrackGameMode TrackPack::getSupportedModes() const {
    auto supportedModes = static_cast<TrackGameMode>(0);
    for (auto mode : modes) {
        if (!getTrackList(mode).empty()) {
            supportedModes |= mode;
        }
    }

    return supportedModes;
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

u16 TrackPack::getTrackCount(TrackGameMode mode) const {
    return getTrackList(mode).size();
}

std::optional<Sha1> TrackPack::getNthTrack(u32 n, TrackGameMode mode) const {
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

TrackPackManager::TrackPackManager() {
    auto res = loadTrackPacks();
    if (!res) {
        panic("Fatal error parsing track packs: %s", res.error());
    }

    loadTrackDb();
}

std::expected<void, const char *> TrackPackManager::loadTrackPacks() {
    SP_LOG("Loading track packs");

    auto *resourceManager = System::ResourceManager::Instance();

    size_t size = 0;
    auto *vanillaManifestRaw = reinterpret_cast<const u8 *>(
            resourceManager->getFile(System::ResChannelId::Menu, "vanillaTracks.pb.bin", &size));
    assert(size != 0);

    std::span vanillaManifest(vanillaManifestRaw, size);
    m_packs.push_back(std::move(TRY(TrackPack::New(vanillaManifest))));

    auto dir = Storage::OpenDir(TRACK_PACK_DIRECTORY);
    if (!dir) {
        SP_LOG("Creating track pack directory");
        Storage::CreateDir(TRACK_PACK_DIRECTORY, true);
        return {};
    }

    std::vector<u8> manifestBuf;
    while (auto nodeInfo = dir->read()) {
        if (nodeInfo->type != Storage::NodeType::File) {
            continue;
        }

        SP_LOG("Found track pack '%ls'", nodeInfo->name);
        manifestBuf.resize(nodeInfo->size);

        auto len = Storage::FastReadFile(nodeInfo->id, manifestBuf.data(), nodeInfo->size);
        if (!len.has_value() || *len == 0) {
            SP_LOG("Failed to read track pack manifest!");
            continue;
        }

        manifestBuf.resize(*len);

        auto res = TrackPack::New(manifestBuf);
        if (!res.has_value()) {
            SP_LOG("Failed to read track pack manifest: %s", res.error());
            continue;
        }

        m_packs.push_back(std::move(*res));
    }

    return {};
}

void TrackPackManager::loadTrackDb() {
    SP_LOG("Loading track DB");

    // Load up the wiimm db, which is pretty large, so we cannot
    // just put it on the stack, so we get the size from stat and
    // allocate a buffer that fits this exact size.
    auto nodeInfo = Storage::Stat(TRACK_DB);
    if (!nodeInfo.has_value()) {
        SP_LOG("No track DB found!");
        return;
    }

    std::vector<u8> trackDbBuf;
    trackDbBuf.resize(nodeInfo->size);

    auto len = Storage::FastReadFile(nodeInfo->id, trackDbBuf.data(), nodeInfo->size);
    if (!len.has_value() || *len == 0) {
        panic("Failed to read track DB!");
    }

    trackDbBuf.resize(*len);

    pb_istream_t stream = pb_istream_from_buffer(trackDbBuf.data(), trackDbBuf.size());

    TrackDB trackDb = TrackDB_init_zero;
    trackDb.tracks.funcs.decode = &decodeTrackCallback;
    trackDb.tracks.arg = &m_trackDb;
    trackDb.aliases.funcs.decode = &DecodeAliasCallback;
    trackDb.aliases.arg = this;
    if (!pb_decode(&stream, TrackDB_fields, &trackDb)) {
        panic("Failed to parse track DB!");
    }

    SP_LOG("Finished loading track DB with %d tracks and %d aliases", m_trackDb.size(),
            m_aliases.size());
}

bool TrackPackManager::anyPackContains(const Sha1 &sha1) {
    auto predicate = [&](TrackPack &pack) { return pack.contains(sha1); };
    return std::find_if(m_packs.begin(), m_packs.end(), predicate) != m_packs.end();
}

const TrackPack &TrackPackManager::getNthPack(u32 n) const {
    return m_packs[n];
}

size_t TrackPackManager::getPackCount() const {
    return m_packs.size();
}

const TrackPack &TrackPackManager::getSelectedTrackPack() const {
    auto *raceConfig = System::RaceConfig::Instance();
    return m_packs[raceConfig->m_selectedTrackPack];
}

const Track *TrackPackManager::getTrackUnaliased(Sha1 sha1) const {
    for (auto &track : m_trackDb) {
        if (track.m_sha1 == sha1) {
            return &track;
        }
    }

    return getSelectedTrackPack().getUnreleasedTrack(sha1);
}

const Track &TrackPackManager::getTrack(Sha1 sha1) const {
    auto unaliasedTrack = getTrackUnaliased(sha1);
    if (unaliasedTrack != nullptr) {
        return *unaliasedTrack;
    }

    auto normalisedSha1 = getNormalisedSha1(sha1);
    if (normalisedSha1.has_value()) {
        auto aliasedTrack = getTrackUnaliased(*normalisedSha1);
        if (aliasedTrack != nullptr) {
            return *aliasedTrack;
        }
    }

    auto hex = sha1ToHex(sha1);
    panic("Unknown sha1 id: %s", hex.data());
}

std::optional<Sha1> TrackPackManager::getNormalisedSha1(Sha1 aliasedSha1) const {
    auto predicate = [&](std::pair<Sha1, Sha1> pair) { return pair.first == aliasedSha1; };
    auto aliasIt = std::find_if(m_aliases.begin(), m_aliases.end(), predicate);

    if (aliasIt == m_aliases.end()) {
        return std::nullopt;
    } else {
        return aliasIt->second;
    }
}

TrackPackManager &TrackPackManager::Instance() {
    if (s_instance == nullptr) {
        panic("TrackPackManager not initialized");
    }

    return *s_instance;
}

void TrackPackManager::CreateInstance() {
    if (s_instance == nullptr) {
        s_instance = new TrackPackManager;
    }
}

void TrackPackManager::DestroyInstance() {
    delete s_instance;
    s_instance = nullptr;
}

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

u32 TrackPackInfo::getSelectedCourse() const {
    return m_selectedCourseId;
}

const wchar_t *TrackPackInfo::getCourseName() const {
    return m_name.c_str();
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

TrackPackManager *TrackPackManager::s_instance = nullptr;

} // namespace SP
