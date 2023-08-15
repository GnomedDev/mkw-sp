#pragma once

#include "vendor/nanopb/pb_decode.h"

bool decodeSha1Callback(pb_istream_t *stream, const pb_field_t * /* field */, void **arg);
