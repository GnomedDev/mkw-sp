#pragma once

#include "sp/storage/Storage.hh"

#include <vendor/nanopb/pb_decode.h>
#include <vendor/nanopb/pb_encode.h>

extern "C" {
#include "revolution.h"
}

#include <expected>
#include <memory>

#include <Common.hh>

namespace SP::Net {

typedef const pb_msgdesc_t *pb_msgdesc_p;

template <typename R, typename W, typename S>
class ProtoSocket {
public:
    // Descriptors would be template param, but blocked by:
    // https://github.com/whitequark/python-itanium_demangler/issues/20
    ProtoSocket(S *socket, pb_msgdesc_p readDesc, pb_msgdesc_p writeDesc) : m_inner(socket) {
        m_readDesc = readDesc;
        m_writeDesc = writeDesc;
    }

    [[nodiscard]] std::expected<std::optional<R>, const wchar_t *> readProto() {
        R ret;
        if (TRY(readProto(&ret))) {
            return ret;
        } else {
            return std::nullopt;
        }
    }

    [[nodiscard]] __attribute__((noinline)) std::expected<bool, const wchar_t *> readProto(R *ret) {
        if (!m_inner->ok()) {
            return std::unexpected(L"Socket has disconnected!");
        }

        u8 buffer[1024];
        std::optional<u16> size = TRY(m_inner->read(buffer, sizeof(buffer)));
        if (!size.has_value()) {
            return false;
        }

        pb_istream_t stream = pb_istream_from_buffer(buffer, *size);
        if (!pb_decode(&stream, m_readDesc, &ret)) {
            SP_LOG("proto error: %s", PB_GET_ERROR(&stream));
            return std::unexpected(L"Failed to decode proto");
        }

        return true;
    }

    [[nodiscard]] __attribute__((noinline)) std::expected<void, const wchar_t *> writeProto(
            W message) {
        if (!m_inner->ok()) {
            return std::unexpected(L"Socket has disconnected!");
        }

        u8 buffer[1024];

        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        assert(pb_encode(&stream, m_writeDesc, &message));
        return m_inner->write(buffer, stream.bytes_written);
    }

    S &inner() {
        return *m_inner;
    }

    const S &inner() const {
        return *m_inner;
    }

private:
    S *m_inner;

    pb_msgdesc_p m_readDesc = nullptr;
    pb_msgdesc_p m_writeDesc = nullptr;
};

} // namespace SP::Net
