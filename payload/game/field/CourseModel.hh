#pragma once

#include <nw4r/g3d/g3d_resmdl_ac.hh>

namespace Field {

class CourseModel {
public:
    REPLACE static void CreateInstance();
    static void DestroyInstance();

    static CourseModel *Instance();

private:
    CourseModel();
    ~CourseModel() = delete;

    static void LoadModels(u32 isLoadingCourse, nw4r::g3d::ResMdl **modelDst, void **param_3,
            const char *modelPath, const char *modelName, u32 param_6);

    void *m_courseSth;
    void *m_skyboxSth;
    nw4r::g3d::ResMdl *m_courseMdl;
    nw4r::g3d::ResMdl *m_skyboxMdl;
    void *_10;

    static CourseModel *s_instance;
};

static_assert(sizeof(CourseModel) == 0x14);

} // namespace Field