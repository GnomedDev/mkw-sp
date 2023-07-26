#include "Update.hh"

extern "C" {
#include "sp/Host.h"
}
#include "sp/net/Net.hh"
#include "sp/net/SyncSocket.hh"

#include <common/Bytes.hh>
#include <common/Paths.hh>

#include <protobuf/Update.pb.h>
extern "C" {
#include <revolution.h>
#include <revolution/nwc24/NWC24Utils.h>
}
#include <vendor/nanopb/pb_decode.h>
#include <vendor/nanopb/pb_encode.h>

#include <algorithm>
#include <cstring>

namespace SP::Update {

#define TMP_CONTENTS_PATH "/tmp/contents.arc"

// clang-format off
static const u8 serverPK[hydro_kx_PUBLICKEYBYTES] = {
    0x73, 0x5f, 0x0c, 0x4b, 0x9f, 0x50, 0xa3, 0xee, 0x9b, 0x0e, 0xb7, 0x8d, 0xe8, 0xc9, 0x6f, 0xcd,
    0xb1, 0xf3, 0xbb, 0x01, 0xa1, 0xf4, 0xa6, 0xb9, 0x32, 0x84, 0x4a, 0x97, 0xa0, 0x5f, 0xbd, 0x22
};
static const u8 signPK[hydro_sign_PUBLICKEYBYTES] = {
    0x8d, 0xa2, 0xdb, 0xfe, 0x59, 0x74, 0x35, 0x92, 0x67, 0x58, 0x45, 0x92, 0x54, 0xd1, 0x5f, 0x8c,
    0x70, 0x67, 0x7b, 0x14, 0x12, 0x74, 0x1d, 0x8e, 0x60, 0xb0, 0x38, 0xbb, 0xf4, 0xbe, 0x34, 0x45
};
// clang-format on
static Status status = Status::Idle;
static std::optional<Info> info;

Status GetStatus() {
    return status;
}

std::optional<Info> GetInfo() {
    return info;
}

static bool Sync(bool update) {
    if (versionInfo.type != BUILD_TYPE_RELEASE) {
        return false;
    }

    SP_LOG("Connecting to localhost");
    status = Status::Connect;
    SP::Net::SyncSocket socket("localhost", 21328, serverPK, "update  ");
    if (!socket.ok()) {
        return false;
    }

    status = Status::SendInfo;
    {
        u8 buffer[UpdateRequest_size];
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

        UpdateRequest request;
        request.wantsUpdate = update;
        request.versionMajor = versionInfo.major;
        request.versionMinor = versionInfo.minor;
        request.versionPatch = versionInfo.patch;
        NWC24iStrLCpy(request.gameName, OSGetAppGamename(), sizeof(request.gameName));
        NWC24iStrLCpy(request.hostPlatform, Host_GetPlatformString(), sizeof(request.hostPlatform));

        assert(pb_encode(&stream, UpdateRequest_fields, &request));

        if (!socket.write(buffer, stream.bytes_written)) {
            return false;
        }
    }

    status = Status::ReceiveInfo;
    {
        u8 buffer[UpdateResponse_size];
        std::optional<u16> size = socket.read(buffer, sizeof(buffer));
        if (!size) {
            return false;
        }

        pb_istream_t stream = pb_istream_from_buffer(buffer, *size);

        UpdateResponse response;
        if (!pb_decode(&stream, UpdateResponse_fields, &response)) {
            return false;
        }

        Info newInfo{};
        newInfo.version.major = response.versionMajor;
        newInfo.version.minor = response.versionMinor;
        newInfo.version.patch = response.versionPatch;
        newInfo.size = response.size;
        if (response.signature.size != sizeof(newInfo.signature)) {
            return false;
        }
        memcpy(newInfo.signature, response.signature.bytes, sizeof(newInfo.signature));
        if (!update) {
            if (newInfo.version > versionInfo) {
                info.emplace(newInfo);
            }
            status = Status::Idle;
            return true;
        } else if (memcmp(&*info, &newInfo, sizeof(Info))) {
            info.reset();
            return false;
        }
    }

    status = Status::Download;
    {
        OSTime startTime = OSGetTime();
        hydro_sign_state state;
        if (hydro_sign_init(&state, "update  ") != 0) {
            return false;
        }
        NANDPrivateDelete(TMP_CONTENTS_PATH);
        u8 perms = NAND_PERM_OWNER_MASK | NAND_PERM_GROUP_MASK | NAND_PERM_OTHER_MASK;
        if (NANDPrivateCreate(TMP_CONTENTS_PATH, perms, 0) != NAND_RESULT_OK) {
            return false;
        }
        NANDFileInfo fileInfo;
        if (NANDPrivateOpen(TMP_CONTENTS_PATH, &fileInfo, NAND_ACCESS_WRITE) != NAND_RESULT_OK) {
            return false;
        }
        for (info->downloadedSize = 0; info->downloadedSize < info->size;) {
            alignas(0x20) u8 message[0x1000] = {};
            u16 chunkSize = std::min(info->size - info->downloadedSize, static_cast<u32>(0x1000));
            if (!socket.read(message, chunkSize)) {
                NANDClose(&fileInfo);
                return false;
            }
            if (hydro_sign_update(&state, message, chunkSize) != 0) {
                NANDClose(&fileInfo);
                return false;
            }
            if (NANDWrite(&fileInfo, message, chunkSize) != chunkSize) {
                NANDClose(&fileInfo);
                return false;
            }
            info->downloadedSize += chunkSize;
            OSTime duration = OSGetTime() - startTime;
            info->throughput = OSSecondsToTicks(static_cast<u64>(info->downloadedSize)) / duration;
        }
        if (NANDClose(&fileInfo) != NAND_RESULT_OK) {
            return false;
        }
        if (hydro_sign_final_verify(&state, info->signature, signPK) != 0) {
            return false;
        }
    }

    status = Status::Move;
    if (NANDPrivateMove(TMP_CONTENTS_PATH, UPDATE_PATH) != NAND_RESULT_OK) {
        return false;
    }

    info->updated = true;
    status = Status::Idle;
    return true;
}

bool Check() {
    if (info) {
        status = Status::Idle;
        return true;
    }
    if (!Sync(false)) {
        SP::Net::Restart();
        return false;
    }
    return true;
}

bool Update() {
    assert(info);
    if (!Sync(true)) {
        info.reset();
        SP::Net::Restart();
        return false;
    }
    return true;
}

} // namespace SP::Update
