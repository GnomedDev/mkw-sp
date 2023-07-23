#include "RaceClient.hh"

#include "sp/cs/RoomClient.hh"

#include <game/kart/KartObjectManager.hh>
#include <game/kart/KartSaveState.hh>
#include <game/system/RaceConfig.hh>
#include <game/system/RaceManager.hh>
#include <vendor/nanopb/pb_decode.h>
#include <vendor/nanopb/pb_encode.h>

#include <cmath>

namespace SP {

void RaceClient::destroyInstance() {
    DestroyInstance();
}

void RaceClient::calcWrite() {
    if (!m_frame) {
        u8 buffer[RaceClientPing_size];
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

        assert(pb_encode(&stream, RaceClientPing_fields, nullptr));

        m_socket.write(buffer, stream.bytes_written, m_connection);
    }

    auto *kartObjectManager = Kart::KartObjectManager::Instance();
    auto &raceScenario = System::RaceConfig::Instance()->raceScenario();
    RoomRequest request;
    request.which_request = RoomRequest_race_tag;
    request.request.race.time = System::RaceManager::Instance()->time();
    request.request.race.serverTime = m_frame ? m_frame->time : 0;
    request.request.race.players_count = raceScenario.localPlayerCount;
    for (u8 i = 0; i < raceScenario.localPlayerCount; i++) {
        u8 playerId = raceScenario.screenPlayerIds[i];
        auto &player = request.request.race.players[i];

        auto kartObject = kartObjectManager->object(playerId);
        auto physics = kartObject->getVehiclePhysics();

        Kart::KartSaveState saveState(kartObject->m_accessor, physics, nullptr);

        player.pos = saveState.m_pos;
        player.externalVel = saveState.m_externalVel;
        player.internalVel = saveState.m_internalVel;
        player.inBullet = saveState.m_inBullet;
        player.mainRot = saveState.m_mainRot;
        player.internalSpeed = saveState.m_internalSpeed;

        player.boostState.timesBeforeEnd_count = 6;
        for (u8 time = 0; time < 6; time += 1) {
            player.boostState.timesBeforeEnd[time] = saveState.m_boostState.m_timesBeforeEnd[time];
        }

        player.boostState.types = saveState.m_boostState.m_types;
        player.boostState.boostMultiplyer = saveState.m_boostState.m_boostMultipler;
        player.boostState.boostAcceleration = saveState.m_boostState.m_boostAcceleration;
        player.boostState.unk_1c = saveState.m_boostState.m_1c;
        player.boostState.boostSpeedLimit = saveState.m_boostState.m_boostSpeedLimit;

        player.wheelPhysics_count = 4;
        for (u8 physics = 0; physics < 4; physics += 1) {
            player.wheelPhysics[physics].realPos = saveState.m_wheelPhysics[physics].m_realPos;
            player.wheelPhysics[physics].lastPos = saveState.m_wheelPhysics[physics].m_lastPos;
            player.wheelPhysics[physics].lastPosDiff =
                    saveState.m_wheelPhysics[physics].m_lastPosDiff;
        }
    }

    u8 buffer[RoomRequest_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

    assert(pb_encode(&stream, RoomRequest_fields, &request));

    // TODO proper error handling
    m_roomClient.socket().write(buffer, stream.bytes_written);
    if (!m_roomClient.socket().poll()) {
        m_roomClient.handleError(30001);
    };
}

void RaceClient::calcRead() {
    ConnectionGroup connectionGroup(*this);

    while (true) {
        u8 buffer[RaceServerFrame_size];
        auto read = m_socket.read(buffer, sizeof(buffer), connectionGroup);
        if (!read) {
            break;
        }

        pb_istream_t stream = pb_istream_from_buffer(buffer, read->size);

        RaceServerFrame frame;
        if (!pb_decode(&stream, RaceServerFrame_fields, &frame)) {
            continue;
        }

        if (isFrameValid(frame)) {
            m_frame = frame;
        }
    }

    if (m_frame) {
        System::RaceManager::Instance()->m_canStartCountdown = true;
    }
}

void RaceClient::applyFrame() {
    if (!m_frame) {
        return;
    }

    auto *kartObjectManager = Kart::KartObjectManager::Instance();
    for (u8 i = 0; i < m_frame->players_count; i += 1) {
        Kart::KartSaveState state{};

        auto &playerState = m_frame->players[i];
        state.m_pos = playerState.pos;
        state.m_externalVel = playerState.externalVel;
        state.m_internalVel = playerState.internalVel;
        state.m_inBullet = playerState.inBullet;
        state.m_mainRot = playerState.mainRot;
        state.m_internalSpeed = playerState.internalSpeed;

        for (u8 time = 0; time < playerState.boostState.timesBeforeEnd_count; time += 1) {
            state.m_boostState.m_timesBeforeEnd[time] = playerState.boostState.timesBeforeEnd[time];
        }

        state.m_boostState.m_types = playerState.boostState.types;
        state.m_boostState.m_boostMultipler = playerState.boostState.boostMultiplyer;
        state.m_boostState.m_boostAcceleration = playerState.boostState.boostAcceleration;
        state.m_boostState.m_1c = playerState.boostState.unk_1c;
        state.m_boostState.m_boostSpeedLimit = playerState.boostState.boostSpeedLimit;

        for (u8 physics = 0; physics < playerState.wheelPhysics_count; physics += 1) {
            state.m_wheelPhysics[physics].m_realPos = playerState.wheelPhysics[physics].realPos;
            state.m_wheelPhysics[physics].m_lastPos = playerState.wheelPhysics[physics].lastPos;
            state.m_wheelPhysics[physics].m_lastPosDiff =
                    playerState.wheelPhysics[physics].lastPosDiff;
        }

        auto *kartObject = kartObjectManager->object(i);
        state.reload(kartObject->m_accessor, kartObject->getVehiclePhysics(), nullptr);
    }
}

RaceClient *RaceClient::CreateInstance() {
    assert(s_block);
    assert(!s_instance);
    auto *roomClient = RoomClient::Instance();
    s_instance = new (s_block) RaceClient(*roomClient);
    RaceManager::s_instance = s_instance;
    return s_instance;
}

void RaceClient::DestroyInstance() {
    assert(s_instance);
    s_instance->~RaceClient();
    RaceManager::s_instance = nullptr;
    s_instance = nullptr;
}

RaceClient *RaceClient::Instance() {
    return s_instance;
}

RaceClient::RaceClient(RoomClient &roomClient)
    : m_roomClient(roomClient), m_socket("race    ", {}),
      m_connection{roomClient.ip(), roomClient.port(), roomClient.keypair()} {}

RaceClient::~RaceClient() {
    hydro_memzero(&m_connection, sizeof(m_connection));
}

bool RaceClient::isFrameValid(const RaceServerFrame &frame) {
    if (m_frame && frame.time <= m_frame->time) {
        return false;
    }

    // TODO check player times
    if (frame.playerTimes_count != m_roomClient.playerCount()) {
        return false;
    }
    if (m_frame) {
        for (u32 i = 0; i < frame.players_count; i++) {
            if (frame.playerTimes[i] < m_frame->playerTimes[i]) {
                return false;
            }
        }
    }

    if (frame.players_count != m_roomClient.playerCount()) {
        return false;
    }
    for (u32 i = 0; i < frame.players_count; i++) {
        if (!IsVec3Valid(frame.players[i].pos)) {
            return false;
        }

        if (!IsQuatValid(frame.players[i].mainRot)) {
            return false;
        }

        if (!IsF32Valid(frame.players[i].internalSpeed)) {
            return false;
        }
    }

    return true;
}

bool RaceClient::IsVec3Valid(const PlayerFrame_Vec3 &v) {
    if (std::isnan(v.x) || v.x < -1e6f || v.x > 1e6f) {
        return false;
    }

    if (std::isnan(v.y) || v.y < -1e6f || v.y > 1e6f) {
        return false;
    }

    if (std::isnan(v.z) || v.z < -1e6f || v.z > 1e6f) {
        return false;
    }

    return true;
}

bool RaceClient::IsQuatValid(const PlayerFrame_Quat &q) {
    if (std::isnan(q.x) || q.x < -1.001f || q.x > 1.001f) {
        return false;
    }

    if (std::isnan(q.y) || q.y < -1.001f || q.y > 1.001f) {
        return false;
    }

    if (std::isnan(q.z) || q.z < -1.001f || q.z > 1.001f) {
        return false;
    }

    if (std::isnan(q.w) || q.w < -1.001f || q.w > 1.001f) {
        return false;
    }

    return true;
}

bool RaceClient::IsF32Valid(f32 s) {
    return !std::isnan(s) && s >= -20.0f && s <= 120.0f;
}

RaceClient::ConnectionGroup::ConnectionGroup(RaceClient &client) : m_client(client) {}

u32 RaceClient::ConnectionGroup::count() {
    return 1;
}

Net::UnreliableSocket::Connection &RaceClient::ConnectionGroup::operator[](u32 index) {
    assert(index == 0);
    return m_client.m_connection;
}

RaceClient *RaceClient::s_instance = nullptr;

} // namespace SP
