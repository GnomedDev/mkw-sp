#include "CourseColiisionManager.hh"

extern "C" {
#include <revolution.h>
}

namespace Field {

nw4r::g3d::ResFile CourseCollisionManager::course() {
    return m_courseResFile;
}

void CourseCollisionManager::hideCourse() {
    for (u32 i = 0; i < m_courseResFile.GetResMdlNumEntries(); ++i) {
        auto mdl = m_courseResFile.GetResMdl(i);
        for (u32 i = 0; i < mdl.GetResNodeNumEntries(); ++i) {
            mdl.GetResNode(i).SetVisibility(false);
        }
    }

    s_isHidden = true;
}

void CourseCollisionManager::showCourse() {
    for (u32 i = 0; i < m_courseResFile.GetResMdlNumEntries(); ++i) {
        auto mdl = m_courseResFile.GetResMdl(i);
        for (u32 i = 0; i < mdl.GetResNodeNumEntries(); ++i) {
            mdl.GetResNode(i).SetVisibility(true);
        }
    }

    s_isHidden = false;
}

bool CourseCollisionManager::isHidden() const {
    return s_isHidden;
}

KclVis *CourseCollisionManager::getVisualisation() {
    return s_kclVis;
}

void CourseCollisionManager::setKclSize(size_t kclSize) {
    assert(s_kclSize == std::numeric_limits<size_t>::max());
    s_kclSize = kclSize;
}

void CourseCollisionManager::init(KCollisionV1Header *file) {
    assert(s_kclSize != std::numeric_limits<size_t>::max());

    REPLACED(init)(file);

    SP_LOG("Allocating KclVis (%u bytes)", sizeof(KclVis));
    std::span<const u8> bytes{(u8 *)file, s_kclSize};
    s_kclVis = new KclVis(bytes);

    s_kclSize = std::numeric_limits<size_t>::max();
}

void CourseCollisionManager::DestroyInstance() {
    s_instance->showCourse();

    REPLACED(DestroyInstance)();
    SP_LOG("Freeing KclVis (%u bytes)", sizeof(KclVis));
    delete s_kclVis;
    s_kclVis = nullptr;
}

CourseCollisionManager *CourseCollisionManager::Instance() {
    return s_instance;
}

bool CourseCollisionManager::s_isHidden = false;
KclVis *CourseCollisionManager::s_kclVis = nullptr;
size_t CourseCollisionManager::s_kclSize = std::numeric_limits<size_t>::max();

} // namespace Field
