#pragma once

#include <egg/core/eggScene.hh>
#include <nw4r/lyt/lyt_arcResourceAccessor.hh>
#include <nw4r/lyt/lyt_drawInfo.hh>
#include <nw4r/lyt/lyt_layout.hh>
#include <nw4r/lyt/lyt_textBox.hh>

namespace System {

enum {
    SCENE_ID_SPFATAL = 100,
};

class FatalScene : public EGG::Scene {
public:
    void calc() override;
    void draw() override;
    void enter() override;
    void exit() override;
    void reinit() override;
    void incoming_childDestroy() override;
    void outgoing_childCreate() override;

    // Set the main text
    void setBody(const wchar_t *body);

    // Take control of rendering
    void mainLoop();

    // If its unsafe to unload the current scene, FatalScene can just repurpose its memory.
    //
    // Calling this + enter() is equivalent to SceneManager::create
    void leechCurrentScene();

private:
    nw4r::lyt::MEMAllocator m_allocator;
    nw4r::lyt::DrawInfo m_drawInfo;

    nw4r::lyt::MultiArcResourceAccessor m_resAccessor;
    nw4r::lyt::ArcResourceLink m_arcLink;

    nw4r::lyt::Layout m_layout;

    nw4r::lyt::TextBox *m_bodyPane;
};

} // namespace System