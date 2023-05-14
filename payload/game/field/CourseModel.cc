#include "CourseModel.hh"

#include "game/field/CourseColiisionManager.hh"
#include "game/system/ResourceManager.hh"

#include <limits>

namespace Field {

CourseModel::CourseModel() {
    m_courseMdl = nullptr;
    m_skyboxMdl = nullptr;

    auto *resourceManager = System::ResourceManager::Instance();

    size_t kclSize = std::numeric_limits<size_t>::max();
    auto *voidFile = resourceManager->getFile(1, "course.kcl", &kclSize);
    if (kclSize == std::numeric_limits<size_t>::max()) {
        panic("Unable to fetch course.kcl from resourceManager!");
    }

    auto *kclFile = static_cast<KCollisionV1Header *>(voidFile);
    CourseCollisionManager::CreateInstance();

    auto *courseColManager = CourseCollisionManager::Instance();
    courseColManager->setKclSize(kclSize);
    courseColManager->init(kclFile);

    auto coursePath = "course_d_model.brres";
    if (System::ResourceManager::ResourceExists(1, coursePath) == 0) {
        coursePath = "course_model.brres";
    }

    auto skyboxPath = "vrcorn_d_model.brres";
    if (System::ResourceManager::ResourceExists(1, skyboxPath) == 0) {
        coursePath = "vrcorn_model.brres";
    }

    LoadModels(1, &m_courseMdl, &m_courseSth, coursePath, "course", 6);
    LoadModels(0, &m_skyboxMdl, &m_skyboxSth, skyboxPath, "vrcorn", 4);
    _10 = nullptr;
}

void CourseModel::CreateInstance() {
    if (s_instance == nullptr) {
        s_instance = new CourseModel;
    }
}

CourseModel *CourseModel::Instance() {
    return s_instance;
}

} // namespace Field
