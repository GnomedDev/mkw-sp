#pragma once

#include "Decoder.hh"

#include <egg/core/eggHeap.hh>

namespace SP {

class WBZDecoder : public Decoder {
public:
    WBZDecoder(const u8 *src, size_t srcSize, EGG::Heap *heap);
    ~WBZDecoder() override;
    bool decode(const u8 *src, size_t size) override;
    void release(u8 **dst, size_t *dstSize) override;
    bool ok() const override;
    bool done() const override;
    size_t headerSize() const override;

    static bool CheckMagic(u32 magic);
};

} // namespace SP
