#pragma once

#include "sp/net/AsyncSocket.hh"
#include "sp/net/AsyncListener.hh"
#include "sp/settings/RoomSettings.hh"

#include <game/system/Mii.hh>
#include <protobuf/Room.pb.h>

#include <array>

namespace SP {

class RoomServer final {
public:
    class Handler {
    public:
        virtual void onMain() {}

        virtual void onPlayerJoin([[maybe_unused]] const System::RawMii *mii,
                [[maybe_unused]] u32 location, [[maybe_unused]] u16 latitude,
                [[maybe_unused]] u16 longitude) {}
        virtual void onPlayerLeave([[maybe_unused]] u32 playerId) {}
        virtual void onReceiveComment([[maybe_unused]] u32 playerId,
                [[maybe_unused]] u32 commentId) {}
        virtual void onSettingsChange(
                [[maybe_unused]] const std::array<u32, RoomSettings::count> &settings) {}
    };

    bool calc(Handler &handler);

    static void OnCreateScene();
    static void OnDestroyScene();
    static RoomServer *CreateInstance();
    static void DestroyInstance();
    static RoomServer *Instance();

private:
    struct Comment {
        u32 playerId;
        u32 messageId;
    };

    struct Player {
        u32 clientId;
        System::RawMii mii;
        u32 location;
        u32 latitude;
        u32 longitude;
        std::array<u32, RoomSettings::count> m_settings;
    };

    enum class State {
        Setup,
        Main,
    };

    class Client {
    public:
        Client(RoomServer &server, u32 id, s32 handle, const hydro_kx_keypair &serverKeypair);
        ~Client();

        bool ready() const;
        bool calc(Handler &handler);

        bool writeJoin(const System::RawMii *mii, u32 location, u32 latitude, u32 longitude);
        bool writeLeave(u32 playerId);
        bool writeComment(u32 playerId, u32 messageId);
        bool writeSettings();

    private:
        enum class State {
            Connect,
            Setup,
            Main,
        };

        std::optional<State> resolve(Handler &handler);
        bool transition(Handler &handler, State state);

        std::optional<State> calcConnect();
        std::optional<State> calcSetup(Handler &handler);
        std::optional<State> calcMain(Handler &handler);

        bool isHost() const;

        bool read(std::optional<RoomRequest> &request);
        bool write(RoomEvent event);

        u32 m_id;
        RoomServer &m_server;
        State m_state;
        Net::AsyncSocket m_socket;
    };

    RoomServer();
    RoomServer(const RoomServer &) = delete;
    RoomServer(RoomServer &&) = delete;
    ~RoomServer();

    std::optional<State> resolve(Handler &handler);
    bool transition(Handler &handler, State state);

    std::optional<State> calcSetup();
    bool onMain(Handler &handler);

    bool onPlayerJoin(Handler &handler, u32 clientId, const System::RawMii *mii, u32 location,
            u16 latitude, u16 longitude, const std::array<u32, RoomSettings::count> &settings);
    void onPlayerLeave(Handler &handler, u32 playerId);
    bool onReceiveComment(u32 playerId, u32 messageId);

    void disconnectClient(u32 clientId);

    void writeJoin(const System::RawMii *mii, u32 location, u32 latitude, u32 longitude);
    void writeLeave(u32 playerId);
    void writeComment(u32 playerId, u32 messageId);
    void writeSettings();

    CircularBuffer<Comment, 18> m_commentQueue;
    u32 m_commentTimer = 0;
    bool m_settingsChanged = false;
    u32 m_playerCount = 0;
    std::array<Player, 12> m_players;
    State m_state;
    CircularBuffer<u32, 12> m_disconnectQueue;
    std::array<std::optional<Client>, 12> m_clients;
    Net::AsyncListener m_listener;
    hydro_kx_keypair m_keypair;

    static void *s_block;
    static RoomServer *s_instance;
};

} // namespace SP
