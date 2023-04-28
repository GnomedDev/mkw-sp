#include "FatalScene.hh"

extern "C" EGG::Scene *SceneCreatorDynamic_createOther(void * /* this */, u32 sceneId) {
    OSReport("SceneCreatorDynamic_createOther %u\n", sceneId);

    switch (sceneId) {
    case System::SCENE_ID_SPFATAL:;
        return new System::FatalScene;
    }

    return nullptr;
}
