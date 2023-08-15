#include "game/ui/Page.hh"

#include <egg/core/eggExpHeap.hh>
#include <egg/core/eggThread.hh>

#include <protobuf/TrackServer.pb.h>
#include <sp/ScopeLock.hh>
#include <sp/ShaUtil.hh>
#include <sp/net/ProtoSocket.hh>
#include <sp/net/SyncSocket.hh>
#include <sp/trackPacks/TrackPack.hh>

#include <vector>

namespace UI {

struct ThreadMessageChannel {
    OSMessageQueue toUI;
    OSMessageQueue toDL;
};

class PackDownloadThread {
public:
    static void *Start(void *args);

private:
    PackDownloadThread(EGG::ExpHeap *heap);

    std::expected<void, const wchar_t *> run(ThreadMessageChannel *queue);
    std::expected<SP::TrackPack, const wchar_t *> downloadPack(Sha1 pack);
    std::expected<void, const wchar_t *> downloadPackTracks(const SP::TrackPack &pack);

    SP::Net::SyncSocket m_innerSocket;
    SP::Net::ProtoSocket<TSTCMessage, CTTSMessage, SP::Net::SyncSocket> m_socket;
    EGG::ExpHeap *m_downloadHeap;
};

class PackDownloadPage : public Page {
public:
    PackDownloadPage();

    void onInit() override;
    void onRefocus() override;
    void afterCalc() override;

    void setToDownload(Sha1 pack);

private:
    bool m_hasErrored;

    std::array<OSMessage, 4> m_messageArray;
    std::array<u8, 1024 * 20> m_threadStack;
    ThreadMessageChannel m_messageQueue;
    OSThread m_thread;

    PageInputManager m_inputManager;
};

} // namespace UI
