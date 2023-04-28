#pragma once

#include "lyt_pane.hh"
#include "lyt_drawInfo.hh"
#include "lyt_arcResourceAccessor.hh"

namespace nw4r::lyt {

class Layout {
public:
    Layout();
    virtual ~Layout();
    virtual void dt(s32 type);

    void calculateMtx(DrawInfo &);
    void draw(DrawInfo &);
    void build(void *buf, MultiArcResourceAccessor *ac);

    inline Rect getLayoutRect() {
        return (Rect){
            .top = m_height / 2,
            .bottom = m_height / 2,
            .left = -m_width / 2,
            .right = m_width / 2,
        };
    }

private:
    u8 _04[0x10 - 0x04];

public:
    Pane *m_rootPane;

private:
    u8 _14[0x18 - 0x14];
    f32 m_width;
    f32 m_height;
};

static_assert(sizeof(Layout) == 0x20);

typedef struct {
    u32 _[4];
} MEMAllocator;
extern MEMAllocator *lyt_spAlloc;

} // namespace nw4r::lyt
