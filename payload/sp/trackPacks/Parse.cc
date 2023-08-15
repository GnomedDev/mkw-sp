#include "Parse.hh"

#include "sp/ShaUtil.hh"

extern "C" {
#include <protobuf/TrackPacks.pb.h>
#include <revolution.h>
}

#include <vector>

bool decodeSha1Callback(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    if (field->submsg_desc != ProtoSha1_fields) {
        SP_LOG("Wrong field type! Found descriptor for %p", field->submsg_desc);
        return false;
    }

    auto &out = *reinterpret_cast<std::vector<Sha1> *>(*arg);

    ProtoSha1 sha1;
    SP_LOG("Decoding SHA1");
    if (!pb_decode(stream, ProtoSha1_fields, &sha1)) {
        SP_LOG("Failed to decode Sha1: %s", PB_GET_ERROR(stream));
        return false;
    }

    assert(sha1.data.size == 0x14);

    out.push_back(std::to_array(sha1.data.bytes));
    return true;
}
