#include "FatalScene.hh"
extern "C" {
#include "FatalScene.h"
}

#include <egg/core/eggColorFader.hh>
#include <egg/core/eggSceneManager.hh>
#include <egg/core/eggSystem.hh>
#include <sp/storage/DecompLoader.hh>
extern "C" {
#include <sp/FlameGraph.h>
}

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// This must always call down to OSFatal (CPU), to prevent infinite recursion
#define OSFATAL_CPU_ASSERT assert

// Read a compressed archive from disc
void *RipFromDiscAlloc(const char *path, EGG::Heap *heap) {
    u8 *szs = NULL;
    size_t szsSize;
    SP::Storage::DecompLoader::LoadRO(path, &szs, &szsSize, heap);
    return szs;
}

nw4r::lyt::Pane *findPane(nw4r::lyt::Layout *lyt, const char *name) {
    auto *result = lyt->m_rootPane->findPaneByName(name, /* recursive */ true);
    if (!result) {
        OSReport("[FatalScene] Failed to find pane %s\n", name);
    }
    return result;
}

void setupGpu() {
    GXSetClipMode(GX_CLIP_ENABLE);
    GXSetCullMode(GX_CULL_NONE);
    GXSetZTexture(GX_ZT_DISABLE, GX_TF_Z8, 0);
    GXSetZMode(GX_FALSE, GX_NEVER, GX_FALSE);

    GXColor fogClr;
    GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, &fogClr);
}

void setupCamera(nw4r::lyt::DrawInfo &drawInfo, nw4r::lyt::Layout &lyt) {
    auto frame = lyt.getLayoutRect();

    f32 znear = -1000.0f;
    f32 zfar = 1000.0f;

    float projMtx[4][4];

    // OSReport("[FatalScene] Ortho Mtx: top=%f,bottom=%f,left=%f,right=%f\n", frame.top,
    //         frame.bottom, frame.left, frame.right);
    C_MTXOrtho(projMtx, frame.top, frame.bottom, frame.left, frame.right, znear, zfar);
    GXSetProjection(projMtx, GX_ORTHOGRAPHIC);

    float viewMtx[3][4];
    PSMTXIdentity(viewMtx);
    static_assert(sizeof(drawInfo.viewMtx) == sizeof(viewMtx));
    memcpy(drawInfo.viewMtx, viewMtx, sizeof(drawInfo.viewMtx));
    drawInfo.viewRect = frame;
}

void PurgeHeap(MEMHeapHandle heap);

extern "C" void free_all_visitor(void *block, MEMHeapHandle heap, u32 /* userParam */) {
    SP_LOG("free_all_visitor");
    MEMHeapHandle child = nullptr;
    for (; (child = static_cast<MEMHeapHandle>(MEMGetNextListObject(&heap->childList, child)));) {
        SP_LOG("iterated");
        PurgeHeap(child);
    }
    if (heap->signature == MEMi_EXPHEAP_SIGNATURE) {
        SP_LOG("freed heap");
        MEMFreeToExpHeap(heap, block);
    }
}

// Does not call dispose
void PurgeHeap(MEMHeapHandle heap) {
    SP_LOG("visiting allocated");
    MEMVisitAllocatedForExpHeap(heap, &free_all_visitor, 0);
    SP_LOG("finished visiting");
    if (heap->signature == MEMi_EXPHEAP_SIGNATURE) {
        MEMDestroyExpHeap(heap);
    }
}

namespace System {

void FatalScene::calc() {}
void FatalScene::draw() {
    //
    // For some reason, the renderMode becomes truncated on the X axis during some loading
    // sequences (?)
    //

    // GXRenderModeObj *rm = sRKSystem.video->renderMode;
    // OSReport("RM: %f, %f\n", (float)rm->fb_width, (float)rm->efb_height);

    setupGpu();
    setupCamera(m_drawInfo, m_layout);

    float w = 640.0f;
    // w = rm->fb_width;
    float h = 456.0f;
    // h = rm->efb_height;

    // OSReport("[FatalScene] Viewport (SYSTEM): w=%f,h=%f\n", (float)rm->fb_width,
    //         (float)rm->efb_height);
    // OSReport("[FatalScene] Viewport: w=%f,h=%f\n", w, h);

    GXSetViewport(0.0f, 0.0f, w, h, 0.0f, 1.0f);
    GXSetScissor(0, 0, (u32)w, (u32)h);
    GXSetScissorBoxOffset(0, 0);

    m_layout.calculateMtx(m_drawInfo);
    m_layout.draw(m_drawInfo);
}

void FatalScene::enter() {
    m_heapMem2->_1c = 0;
    m_heapMem2->initAllocator(&m_allocator, 32);
    lyt_spAlloc = &m_allocator;
    m_heapMem2->_1c = 0;

    SP_LOG("initalised allocator");

    void *archive = RipFromDiscAlloc("Scene/UI/CrashSP.szs", m_heapMem2);
    OSFATAL_CPU_ASSERT(archive);

    SP_LOG("loaded archive");

    m_arcLink.set(archive, ".");
    m_resAccessor.attach(m_arcLink);

    SP_LOG("done shit");

    {
        void *lytRes = m_resAccessor.getResource(0, "Fatal.brlyt", nullptr);
        OSFATAL_CPU_ASSERT(lytRes && "Can't find `Fatal.brlyt`");
        m_layout.build(lytRes, &m_resAccessor);
    }

    SP_LOG("built layout");

    m_bodyPane = static_cast<nw4r::lyt::TextBox *>(findPane(&m_layout, "Body"));
    OSFATAL_CPU_ASSERT(m_bodyPane && "Can't find `Body` pane in brlyt");

    SP_LOG("found bodypane");

    OSFATAL_CPU_ASSERT(m_sceneManager);
    OSFATAL_CPU_ASSERT(m_sceneManager->fader);
    m_sceneManager->fader->fadeIn();

    SP_LOG("fading in");
}

void FatalScene::setBody(const wchar_t *body) {
    OSFATAL_CPU_ASSERT(m_bodyPane);
    if (m_bodyPane != NULL) {
        m_bodyPane->setString(body, 0);
    }
}

void FatalScene::exit() {}
void FatalScene::reinit() {}
void FatalScene::incoming_childDestroy() {}
void FatalScene::outgoing_childCreate() {}

void FatalScene::leechCurrentScene() {
    auto *sceneManager = EGG::TSystem::Instance()->getSceneManager();

    // Reclaim MEM2
    auto *curScene = sceneManager->m_currScene;
    SP_LOG("cur scene");
    auto mem2 = curScene->m_heapMem2;
    SP_LOG("mem2");
    auto heapHandle = mem2->heapHandle;
    SP_LOG("purge heap");
    PurgeHeap(heapHandle);
    SP_LOG("finished purging");
    m_heapMem2 = curScene->m_heapMem2;

    // Unlock it
    m_heapMem2->_1c = 0;

    m_sceneManager = sceneManager;
    m_parent = curScene;
}

void FatalScene::mainLoop() {
    auto *display = EGG::TSystem::Instance()->getDisplay();

    for (;;) {
        display->vt->beginFrame(display);
        display->vt->beginRender(display);

        calc();
        draw();

        display->vt->endRender(display);
        display->vt->endFrame(display);
    }
}

} // namespace System
