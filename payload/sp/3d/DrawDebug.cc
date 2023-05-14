#include "DrawDebug.hh"

#include <game/field/CourseColiisionManager.hh>
#include <game/render/DrawList.hh>
#include <game/system/SaveManager.hh>

namespace SP {

using DebugKCL = SP::ClientSettings::DebugKCL;

void DrawDebug() {
    auto *courseCollisionManager = Field::CourseCollisionManager::Instance();
    if (!courseCollisionManager) {
        return;
    }

    auto *visualisation = courseCollisionManager->getVisualisation();
    if (!visualisation) {
        return;
    }

    auto *saveManager = System::SaveManager::Instance();
    auto setting = saveManager->getSetting<SP::ClientSettings::Setting::DebugKCL>();

    bool isHidden = courseCollisionManager->isHidden();
    if (setting == DebugKCL::Replace && !isHidden) {
        SP_LOG("Hiding course");
        courseCollisionManager->hideCourse();
    } else if (setting != DebugKCL::Replace && isHidden) {
        SP_LOG("Showing course");
        courseCollisionManager->showCourse();
    }

    if (setting == DebugKCL::Overlay || setting == DebugKCL::Replace) {
        const std::array<float, 12> mtx = Render::DrawList::spInstance->getViewMatrix();
        visualisation->render(Decay(mtx), setting == DebugKCL::Overlay);
    }
}

} // namespace SP
