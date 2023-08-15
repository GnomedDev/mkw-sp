#include "PackDownloadPage.hh"

#include "game/ui/MessagePage.hh"
#include "game/ui/SectionManager.hh"

#include <game/system/GameScene.hh>
#include <sp/storage/Storage.hh>
#include <sp/trackPacks/Parse.hh>
#include <sp/trackPacks/TrackPackManager.hh>
extern "C" {
#include <sp/Commands.h>
}

namespace UI {

// Must use the lock to read or write {
static SP::Mutex s_sharedLock = {};
static const wchar_t *s_threadError = nullptr;
static std::optional<Sha1> s_downloadPack = std::nullopt;
static std::optional<SP::TrackPack> s_downloadedPack = std::nullopt;
static std::optional<SP::TrackPack> s_downloadPackTracks = std::nullopt;
// }

enum class ThreadMessage {
    NoMessage = 0,
    // UI -> Thread messages
    DownloadPack,
    DownloadPackTracks,
    Shutdown,
    // Thread -> UI messages
    FinishedDownloadPack,
    FinishedDownloadPackTracks,
    Errored,
};

OSMessage toMessage(ThreadMessage msg) {
    static_assert(sizeof(ThreadMessage) == 0x4);
    return reinterpret_cast<OSMessage>(msg);
}

// clang-format off
static u8 PACK_SERVER_PK[] = {
    0xc3, 0x77, 0x36, 0xed, 0x60, 0x6e, 0x00, 0xe6, 0xc3, 0x99, 0xc2, 0x72, 0xfe, 0xc4, 0xcd, 0xa3,
    0x24, 0x58, 0x03, 0x90, 0x60, 0x96, 0x24, 0x01, 0xf0, 0x81, 0x5e, 0x0f, 0xc1, 0x35, 0x1a, 0x21
};
// clang-format on

bool writePacks(pb_ostream_t *stream, const pb_field_t *field, void *const * /* arg */) {
    auto &trackPackManager = SP::TrackPackManager::Instance();

    ProtoSha1 protoSha1;
    for (size_t i = 0; i < trackPackManager.getPackCount(); i += 1) {
        if (!pb_encode_tag_for_field(stream, field)) {
            return false;
        }

        auto &pack = trackPackManager.getNthPack(i);

        protoSha1.data.size = sizeof(Sha1);
        memcpy(&protoSha1.data.bytes, pack.getManifestSha().data(), sizeof(Sha1));

        if (!pb_encode_submessage(stream, ProtoSha1_fields, &protoSha1)) {
            return false;
        }
    }

    return true;
}

bool readUpdates(pb_istream_t *stream, const pb_field_t * /* field */, void **arg) {
    auto *out = *reinterpret_cast<std::vector<UpdatePath> **>(arg);

    SP_LOG("readUpdates");
    UpdatePath update = UpdatePath_init_zero;
    if (pb_decode(stream, UpdatePath_fields, &update)) {
        out->push_back(std::move(update));
        return true;
    } else {
        return false;
    }
}

bool readLoginResponse(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    SP_LOG("reading login response");
    if (field->submsg_desc != TSTCMessage_LoginResponse_fields) {
        SP_LOG("Not login response");
        return false;
    }

    TSTCMessage_LoginResponse message;
    message.packUpdates.arg = NULL;
    message.packUpdates.funcs.decode = readUpdates;
    message.topPacks.arg = *arg;
    message.topPacks.funcs.decode = decodeSha1Callback;

    return pb_decode(stream, TSTCMessage_LoginResponse_fields, &message);
}

PackDownloadPage::PackDownloadPage() = default;

void PackDownloadPage::onInit() {
    SP::TrackPackManager::CreateInstance();

    auto *mem2Heap = System::GameScene::Instance()->volatileHeapCollection.mem2;
    auto *downloadHeap = EGG::ExpHeap::Create(100'000, mem2Heap, 0);

    size_t stackSize = m_threadStack.size();
    u8 *stackTop = m_threadStack.data() + stackSize;

    auto &messageQueue = m_messageQueue;
    OSInitMessageQueue(&messageQueue.toDL, m_messageArray.data(), 2);
    OSInitMessageQueue(&messageQueue.toUI, m_messageArray.data() + 2, 2);
    OSSendMessage(&messageQueue.toDL, downloadHeap, OS_MESSAGE_BLOCK);

    OSCreateThread(&m_thread, PackDownloadThread::Start, &messageQueue, stackTop, stackSize, 20, 0);
    OSResumeThread(&m_thread);

    m_inputManager.init(0, 0);
    setInputManager(&m_inputManager);
    initChildren(0);
}

void PackDownloadPage::onRefocus() {
    if (m_hasErrored) {
        changeSection(SectionId::TitleFromOptions, Anim::Prev, 0.0f);
    }
}

void PackDownloadPage::afterCalc() {
    ThreadMessage message;
    if (!OSReceiveMessage(&m_messageQueue.toUI, reinterpret_cast<OSMessage *>(&message),
                OS_MESSAGE_NOBLOCK)) {
        return;
    }

    SP::ScopeLock<SP::Mutex> _guard = s_sharedLock;
    assert(message != ThreadMessage::NoMessage);

    if (message != ThreadMessage::Errored) {
        SP_LOG("Message recieved that isn't errored");
        return;
    }

    assert(s_threadError != nullptr);

    SP_LOG("Pack download error: %ls", s_threadError);
    m_hasErrored = true;

    auto *section = SectionManager::Instance()->currentSection();
    auto *messagePopup = section->page<PageId::MessagePopup>();

    messagePopup->reset();
    messagePopup->setWindowMessage(30004);
    push(PageId::MessagePopup, Anim::None);
}

void PackDownloadPage::setToDownload(Sha1 hash) {
    {
        SP::ScopeLock<SP::Mutex> _guard = s_sharedLock;
        s_downloadPack = hash;
    }

    OSSendMessage(&m_messageQueue.toDL, toMessage(ThreadMessage::DownloadPack), OS_MESSAGE_BLOCK);
}

PackDownloadThread::PackDownloadThread(EGG::ExpHeap *heap)
    : m_innerSocket{"localhost", 21333, PACK_SERVER_PK, "trackpak"},
      m_socket(&m_innerSocket, TSTCMessage_fields, CTTSMessage_fields), m_downloadHeap(heap) {}

void *PackDownloadThread::Start(void *argsErased) {
    auto queue = static_cast<ThreadMessageChannel *>(argsErased);

    EGG::ExpHeap *downloadHeap;
    OSReceiveMessage(&queue->toDL, reinterpret_cast<OSMessage *>(&downloadHeap), OS_MESSAGE_BLOCK);

    PackDownloadThread self = {downloadHeap};
    std::expected<void, const wchar_t *> result = self.run(queue);

    if (result.has_value()) {
        return nullptr;
    }

    {
        SP::ScopeLock<SP::Mutex> _guard = s_sharedLock;
        s_threadError = result.error();
    }

    OSSendMessage(&queue->toUI, toMessage(ThreadMessage::Errored), OS_MESSAGE_BLOCK);
    return nullptr;
}

std::expected<void, const wchar_t *> PackDownloadThread::run(ThreadMessageChannel *queue) {
    std::vector<ProtoSha1> topPacks;
    std::vector<UpdatePath> outdatedPacks;

    {
        CTTSMessage outMessage = CTTSMessage_init_zero;

        std::optional<SP::TrackPack> queuedPack;
        outMessage.which_message = CTTSMessage_login_tag;
        outMessage.message.login.downloadedPacks.arg = nullptr;
        outMessage.message.login.downloadedPacks.funcs.encode = writePacks;
        SP_LOG("Sending login message");
        TRY(m_socket.writeProto(outMessage));
    }

    {
        TSTCMessage inMessage = TSTCMessage_init_zero;

        inMessage.cb_message.funcs.decode = readLoginResponse;
        inMessage.cb_message.arg = &topPacks;
        SP_LOG("Reading login response");
        TRY(m_socket.readProto(&inMessage));
    }

    while (true) {
        ThreadMessage threadMessage;
        OSReceiveMessage(&queue->toDL, reinterpret_cast<OSMessage *>(&threadMessage),
                OS_MESSAGE_BLOCK);
        if (threadMessage == ThreadMessage::Shutdown) {
            break;
        } else if (threadMessage == ThreadMessage::DownloadPack) {
            std::optional<Sha1> packToDownload;

            {
                SP::ScopeLock<SP::Mutex> _guard = s_sharedLock;
                packToDownload = s_downloadPack.value();
            }

            auto pack = TRY(downloadPack(*packToDownload));

            {
                SP::ScopeLock<SP::Mutex> _guard = s_sharedLock;
                s_downloadedPack = pack;
            }

            auto message = toMessage(ThreadMessage::FinishedDownloadPack);
            OSSendMessage(&queue->toUI, message, OS_MESSAGE_BLOCK);
        } else if (threadMessage == ThreadMessage::DownloadPackTracks) {
            std::optional<SP::TrackPack> tracksToDownload;

            {
                SP::ScopeLock<SP::Mutex> _guard = s_sharedLock;
                tracksToDownload = s_downloadPackTracks.value();
            }

            TRY(downloadPackTracks(*tracksToDownload));

            auto message = toMessage(ThreadMessage::FinishedDownloadPackTracks);
            OSSendMessage(&queue->toUI, message, OS_MESSAGE_BLOCK);
        }
    };

    return {};
}

std::expected<SP::TrackPack, const wchar_t *> PackDownloadThread::downloadPack(Sha1 packSha) {
    CTTSMessage packDownMsg = CTTSMessage_init_zero;
    packDownMsg.which_message = CTTSMessage_download_pack_tag;
    packDownMsg.message.download_pack.packId.data.size = sizeof(Sha1);
    memcpy(packDownMsg.message.download_pack.packId.data.bytes, packSha.data(), sizeof(Sha1));
    TRY(m_socket.writeProto(packDownMsg));

    TSTCMessage responseMsg = TRY(m_socket.readProto()).value();
    if (responseMsg.which_message != TSTCMessage_pack_response_tag) {
        return std::unexpected(L"Wrong response message to pack download request");
    }

    auto packSize = responseMsg.message.pack_response.packSize;
    u8 *packBufPtr = reinterpret_cast<u8 *>(m_downloadHeap->alloc(packSize, alignof(Pack)));

    u8 chunkBuf[1024];
    u32 currentOffset = 0;
    size_t bytesLeft = packSize;
    while (bytesLeft > 0) {
        auto chunk = std::min(static_cast<u32>(bytesLeft), sizeof(chunkBuf));

        memcpy(packBufPtr + currentOffset, chunkBuf, chunk);

        currentOffset += chunk;
        bytesLeft = packSize - currentOffset;
    }

    std::span<u8> packBuf(packBufPtr, packSize);
    auto res = SP::TrackPack::New(packBuf);
    if (res.has_value()) {
        return *res;
    } else {
        return std::unexpected(L"Failed to download Pack Manifest");
    }
}

std::expected<void, const wchar_t *> PackDownloadThread::downloadPackTracks(
        const SP::TrackPack &pack) {
    const wchar_t *error = nullptr;
    pack.forEachTrack([&](const Sha1 track) {
        if (error != nullptr) {
            return;
        }

        wchar_t trackPath[64];
        auto trackHex = sha1ToHex(track);
        swprintf(trackPath, sizeof(trackPath), L"Tracks/%s.wbz", trackHex.data());
        auto outFile = SP::Storage::Open(trackPath, "w");
        if (!outFile.has_value()) {
            error = L"Failed to open output file";
            return;
        }

        {
            CTTSMessage outMessage = CTTSMessage_init_zero;
            outMessage.which_message = CTTSMessage_download_track_tag;
            outMessage.message.download_track.trackId.data.size = sizeof(Sha1);
            memcpy(outMessage.message.download_track.trackId.data.bytes, track.data(),
                    sizeof(Sha1));

            auto res = m_socket.writeProto(outMessage);
            if (!res.has_value()) {
                error = res.error();
                return;
            }
        }

        auto res = m_socket.readProto();
        if (!res.has_value() || !*res) {
            error = L"Failed to download track";
            return;
        };

        auto trackMeta = **res;
        if (trackMeta.which_message != TSTCMessage_track_response_tag) {
            error = L"Wrong message recieved";
            return;
        }

        u8 trackBuf[1024];
        u32 currentOffset = 0;
        s32 bytesLeft = trackMeta.message.track_response.trackSize;
        while (bytesLeft > 0) {
            auto chunk = std::min(static_cast<u32>(bytesLeft), sizeof(trackBuf));
            auto res = m_innerSocket.read(trackBuf, chunk);
            if (!res) {
                error = L"Failed to recieve track";
                return;
            }

            assert(outFile->write(trackBuf, chunk, currentOffset));

            currentOffset += chunk;
            bytesLeft = trackMeta.message.track_response.trackSize - currentOffset;
        }
    });

    return {};
}

} // namespace UI

sp_define_command("/download_pack", "Download a pack", const char *tmp) {
    tmp = "/download_pack 10d8f73b92b77f98b6598730ab9a3d5c4468f8a2";

    char hashBuf[0x14 * 2];
    if (!sscanf(tmp, "/download_pack %s", hashBuf)) {
        SP_LOG("Invalid argument!");
        return;
    }

    std::string_view hashBufSv(hashBuf, 0x14 * 2);
    auto hash = sha1FromHex(hashBufSv);
    if (!hash.has_value()) {
        SP_LOG("Invalid hash: %s", hash.error());
        return;
    }

    auto *page = UI::SectionManager::Instance()->currentSection()->page<UI::PageId::PackDownload>();
    page->setToDownload(*hash);

    SP_LOG("Queued pack for download");
}
