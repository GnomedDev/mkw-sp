#pragma once

#include <protobuf/TrackPacks.pb.h>
#include <vendor/nanopb/pb_decode.h>

bool decodeSha1Callback(pb_istream_t *stream, const pb_field_t * /* field */, void **arg);
bool decodeTrackCallback(pb_istream_t *stream, const pb_field_t * /* field */, void **arg);
bool decodeAliasCallback(pb_istream_t *stream, const pb_field_t * /* field */, void **arg);