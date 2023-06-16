#include "TrackPackManager.hh"
#include "Parse.hh"

#include "sp/storage/Storage.hh"

#include <game/system/RaceConfig.hh>
#include <game/system/ResourceManager.hh>

namespace SP {

#define TRACK_PACK_DIRECTORY L"Track Packs"
#define TRACK_DB L"TrackDB.pb.bin"

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
    trackDb.aliases.funcs.decode = &decodeAliasCallback;
    trackDb.aliases.arg = this;
    if (!pb_decode(&stream, TrackDB_fields, &trackDb)) {
        panic("Failed to parse track DB!");
    }

    SP_LOG("Finished loading track DB with %d tracks and %d aliases", m_trackDb.size(),
            m_aliases.size());
}

size_t TrackPackManager::getPackCount() const {
    return m_packs.size();
}

const Track &TrackPackManager::getTrack(Sha1 sha1) const {
    for (auto &track : m_trackDb) {
        if (track.m_sha1 == sha1) {
            return track;
        }
    }

    auto track = getSelectedTrackPack().getUnreleasedTrack(sha1);
    if (track != nullptr) {
        return *track;
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

const TrackPack &TrackPackManager::getNthPack(u32 n) const {
    return m_packs[n];
}

const TrackPack &TrackPackManager::getSelectedTrackPack() const {
    auto *raceConfig = System::RaceConfig::Instance();
    return m_packs[raceConfig->m_selectedTrackPack];
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

bool TrackPackManager::anyPackContains(const Sha1 &sha1) {
    auto predicate = [&](TrackPack &pack) { return pack.contains(sha1); };
    return std::find_if(m_packs.begin(), m_packs.end(), predicate) != m_packs.end();
}

TrackPackManager *TrackPackManager::s_instance = nullptr;

} // namespace SP
