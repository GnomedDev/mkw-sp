#pragma once

#include "game/field/Kcl.hh"

#include <nw4r/g3d/g3d_resmdl_ac.hh>

#include <limits>

namespace Field {

class CourseCollisionManager {
public:
    // Must be called before `init`, would be passed in as an argument
    // but then the REPLACE and REPLACED symbol would be mismatched.
    void setKclSize(size_t kclSize);

    void REPLACED(init)(KCollisionV1Header *kclFile);
    REPLACE void init(KCollisionV1Header *kclFile);

    nw4r::g3d::ResFile course();
    void hideCourse();
    void showCourse();
    bool isHidden() const;
    KclVis *getVisualisation();

    static void CreateInstance();
    static void REPLACED(DestroyInstance)();
    REPLACE static void DestroyInstance();
    static CourseCollisionManager *Instance();

private:
    CourseCollisionManager();
    ~CourseCollisionManager();

    u32 _00;
    u32 _04;
    nw4r::g3d::ResFile m_courseResFile;
    nw4r::g3d::ResFile m_vrcornResFile;

    static bool s_isHidden;
    static size_t s_kclSize;
    static KclVis *s_kclVis;

    static CourseCollisionManager *s_instance;
};

static_assert(sizeof(CourseCollisionManager) == 0x10);

} // namespace Field
