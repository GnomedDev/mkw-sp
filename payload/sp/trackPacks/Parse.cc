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

bool decodeTrackCallback(pb_istream_t *stream, const pb_field_t * /* field */, void **arg) {
    auto &out = *reinterpret_cast<std::vector<SP::Track> *>(*arg);

    ProtoTrack protoTrack;
    if (!pb_decode(stream, ProtoTrack_fields, &protoTrack)) {
        panic("Failed to decode track: %s", PB_GET_ERROR(stream));
    }

    assert(protoTrack.sha1.data.size == 0x14);
    Sha1 sha1 = std::to_array(protoTrack.sha1.data.bytes);

    out.emplace_back(sha1, protoTrack.slotId, protoTrack.type == 2, (const char *)&protoTrack.name);

    if (protoTrack.has_musicId) {
        out.back().m_musicId = protoTrack.musicId;
    }

    return true;
}

bool decodeAliasCallback(pb_istream_t *stream, const pb_field_t * /* field */, void **arg) {
    auto &out = *reinterpret_cast<std::vector<std::pair<Sha1, Sha1>> *>(*arg);

    TrackDB_AliasValue protoAlias;
    if (!pb_decode(stream, TrackDB_AliasValue_fields, &protoAlias)) {
        panic("Failed to decode alias: %s", PB_GET_ERROR(stream));
    }

    assert(protoAlias.aliased.data.size == 0x14);
    Sha1 aliased = std::to_array(protoAlias.aliased.data.bytes);

    assert(protoAlias.real.data.size == 0x14);
    Sha1 real = std::to_array(protoAlias.real.data.bytes);

    out.emplace_back(aliased, real);
    return true;
}