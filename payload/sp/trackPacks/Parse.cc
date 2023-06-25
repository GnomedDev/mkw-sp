#include "Parse.hh"
#include "Track.hh"

#include <protobuf/TrackPacks.pb.h>
#include <vector>

bool decodeSha1Callback(pb_istream_t *stream, const pb_field_t * /* field */, void **arg) {
    auto &out = *reinterpret_cast<std::vector<Sha1> *>(*arg);

    ProtoSha1 sha1;
    if (!pb_decode(stream, ProtoSha1_fields, &sha1)) {
        panic("Failed to decode Sha1: %s", PB_GET_ERROR(stream));
    }

    assert(sha1.data.size == 0x14);

    out.push_back(std::to_array(sha1.data.bytes));
    return true;
}
