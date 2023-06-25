#include "TrackPackManager.hh"
#include "Parse.hh"

#include "sp/storage/Storage.hh"

#include <game/system/RaceConfig.hh>
#include <game/system/ResourceManager.hh>

namespace SP {

#define TRACK_PACK_DIRECTORY L"Track Packs"
#define TRACK_DIRECTORY L"Tracks"

std::span<u8> readVanillaTrack(std::array<char, 0x28 + 1> sha1Hex) {
    auto *resourceManager = System::ResourceManager::Instance();

    char pathBuf[sizeof("vanillaTracks/") + sizeof(sha1Hex) + sizeof(".pb.bin")];
    snprintf(pathBuf, std::size(pathBuf), "vanillaTracks/%s.pb.bin", sha1Hex.data());

    size_t size = 0;
    void *manifest = resourceManager->getFile(System::ResChannelId::Menu, pathBuf, &size);
    if (size == 0) {
        panic("Failed to load vanilla track metadata for %s", sha1Hex.data());
    }

    return std::span(reinterpret_cast<u8 *>(manifest), size);
}

std::span<u8> readSDTrack(std::vector<u8> &manifestBuf, std::array<char, 0x28 + 1> sha1Hex) {
    char pathBuf[sizeof("mkw-sp/Tracks/") + sizeof(sha1Hex) + sizeof(".pb.bin")];
    snprintf(pathBuf, std::size(pathBuf), "mkw-sp/Tracks/%s.pb.bin", sha1Hex.data());

    auto trackHandle = Storage::OpenRO(pathBuf);
    if (!trackHandle.has_value()) {
        panic("Could not find track metadata for %s", sha1Hex.data());
    }

    manifestBuf.resize(trackHandle->size());
    if (!trackHandle->read(manifestBuf.data(), manifestBuf.size(), 0)) {
        panic("Could not read track metadata for %s", sha1Hex.data());
    }

    return std::span(manifestBuf.data(), manifestBuf.size());
}

TrackPackManager::TrackPackManager() {
    auto res = loadTrackPacks();
    if (!res) {
        panic("Fatal error parsing track packs: %s", res.error());
    }

    loadTrackMetadata();
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

void TrackPackManager::loadTrackMetadata() {
    SP_LOG("Loading track metadata");

    auto dir = Storage::OpenDir(TRACK_DIRECTORY);
    assert(dir.has_value()); // Was created in loadTrackPacks

    bool foundVanilla = false;
    std::span<u8> manifestView;
    std::vector<u8> manifestBuf;
    for (auto &pack : m_packs) {
        SP_LOG("Loading track metadata for pack: %ls", pack.getPrettyName());
        for (auto mode : s_trackModes) {
            std::optional<Sha1> trackSha;
            for (u16 i = 0; (trackSha = pack.getNthTrack(i, mode)); i += 1) {
                auto trackShaHex = sha1ToHex(trackSha.value());
                if (foundVanilla) {
                    manifestView = readSDTrack(manifestBuf, trackShaHex);
                } else {
                    manifestView = readVanillaTrack(trackShaHex);
                }

                loadTrack(manifestView, *trackSha);
            }
        }

        foundVanilla = true;
    }
}

void TrackPackManager::loadTrack(std::span<u8> manifestBuf, Sha1 sha1) {
    ProtoTrack protoTrack;
    pb_istream_t stream = pb_istream_from_buffer(manifestBuf.data(), manifestBuf.size());
    if (!pb_decode(&stream, ProtoTrack_fields, &protoTrack)) {
        panic("Failed to decode track: %s", PB_GET_ERROR(&stream));
    }

    m_trackDb.emplace_back(sha1, protoTrack.slotId, protoTrack.type == 2,
            (const char *)&protoTrack.name);

    if (protoTrack.has_musicId) {
        m_trackDb.back().m_musicId = protoTrack.musicId;
    }
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
