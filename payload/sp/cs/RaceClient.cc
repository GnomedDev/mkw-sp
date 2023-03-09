#include "RaceClient.hh"

#include "sp/cs/RoomClient.hh"

#include <game/kart/KartObjectManager.hh>
#include <game/system/RaceConfig.hh>
#include <game/system/RaceManager.hh>
#include <vendor/nanopb/pb_decode.h>
#include <vendor/nanopb/pb_encode.h>

#include <cmath>

namespace SP {

void RaceClient::destroyInstance() {
    DestroyInstance();
}

u32 RaceClient::frameCount() const {
    return m_frameCount;
}

const std::optional<RoomEvent_RaceServerFrame> &RaceClient::frame() const {
    return m_frame;
}

/*s32 RaceClient::drift() const {
    return m_drift;
}

void RaceClient::adjustDrift() {
    if (m_drift == 0) {
        return;
    }

    s32 signum = (m_drift > 0) - (m_drift < 0);
    for (size_t i = 0; i < m_drifts.count(); i++) {
        *m_drifts[i] -= signum;
    }
    m_drift -= signum;
}*/

void RaceClient::calcWrite() {
    if (!m_frame) {
        u8 buffer[RoomRequest_RaceClientPing_size];
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

        assert(pb_encode(&stream, RoomRequest_RaceClientPing_fields, nullptr));

        m_socket.write(buffer, stream.bytes_written, m_connection);
    }

    auto &raceScenario = System::RaceConfig::Instance()->raceScenario();
    RoomRequest request;
    request.which_request = RoomRequest_race_tag;
    request.request.race.time = System::RaceManager::Instance()->time();
    request.request.race.serverTime = m_frame ? m_frame->time : 0;
    request.request.race.players_count = raceScenario.localPlayerCount;
    for (u8 i = 0; i < raceScenario.localPlayerCount; i++) {
        u8 playerId = raceScenario.screenPlayerIds[i];
        auto *player = System::RaceManager::Instance()->player(playerId);
        auto &inputState = player->padProxy()->currentRaceInputState();
        request.request.race.players[i].inputState.accelerate = inputState.accelerate;
        request.request.race.players[i].inputState.brake = inputState.brake;
        request.request.race.players[i].inputState.item = inputState.item;
        request.request.race.players[i].inputState.drift = inputState.drift;
        request.request.race.players[i].inputState.brakeDrift = inputState.brakeDrift;
        request.request.race.players[i].inputState.stickX = inputState.rawStick.x;
        request.request.race.players[i].inputState.stickY = inputState.rawStick.y;
        request.request.race.players[i].inputState.trick = inputState.rawTrick;
        auto *object = Kart::KartObjectManager::Instance()->object(playerId);
        request.request.race.players[i].timeBeforeRespawn = object->getTimeBeforeRespawn();
        request.request.race.players[i].timeInRespawn = object->getTimeInRespawn();
        request.request.race.players[i].timesBeforeBoostEnd_count = 3;
        for (u32 j = 0; j < 3; j++) {
            request.request.race.players[i].timesBeforeBoostEnd[j] =
                    object->getTimeBeforeBoostEnd(j * 2);
        }
        request.request.race.players[i].pos = *object->getPos();
        request.request.race.players[i].mainRot = *object->getMainRot();
        request.request.race.players[i].internalSpeed = object->getInternalSpeed();
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
    while (true) {
        u8 buffer[RoomEvent_RaceServerFrame_size];
        auto read = m_socket.read(buffer, sizeof(buffer), connectionGroup);
        if (!read) {
            break;
        }

        pb_istream_t stream = pb_istream_from_buffer(buffer, read->size);

        RoomEvent_RaceServerFrame frame;
        if (!pb_decode(&stream, RoomEvent_RaceServerFrame_fields, &frame)) {
            continue;
        }

        if (isFrameValid(frame)) {
            m_frameCount++;
            m_frame = frame;
        }
    }

    if (!m_frame) {
        return;
    }

    System::RaceManager::Instance()->m_canStartCountdown = true;

    /*if (m_drifts.full()) {
        m_drifts.pop_front();
    }
    s32 drift = static_cast<s32>(m_frame->clientTime) - static_cast<s32>(m_frame->time);
    m_drifts.push_back(std::move(drift));

    m_drift = 0;
    for (size_t i = 0; i < m_drifts.count(); i++) {
        m_drift += *m_drifts[i];
    }
    m_drift /= static_cast<s32>(m_drifts.count());*/
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

RaceClient::RaceClient(RoomClient &roomClient) : m_roomClient(roomClient), m_socket("race    ", {}),
        m_connection{roomClient.ip(), roomClient.port(), roomClient.keypair()} {}

RaceClient::~RaceClient() {
    hydro_memzero(&m_connection, sizeof(m_connection));
}


RaceClient *RaceClient::s_instance = nullptr;

} // namespace SP
