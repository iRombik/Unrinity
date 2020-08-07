#pragma once
#include "glm/glm.hpp"
#include "Components/transformation.h"
#include "Components/lightSource.h"

namespace EFFECT_DATA {
    enum SHADER_ID {
        SHR_BASE = 0,
        SHR_FULLSCREEN = 1,
        SHR_SHADOW = 2,
        SHR_TERRAIN = 3,
        SHR_UI = 4,
        SHR_LAST
    };

    enum SHADER_TYPE {
        VERTEX = 0,
        PIXEL = 1,
        LAST = 2
    };
}

namespace EFFECT_DATA
{
    enum SAMPLERS {
        REPEAT_LINEAR_ANISO = 0,
        REPEAT_LINEAR       = 1,
        LAST                = 2
    };
}

namespace EFFECT_DATA 
{
    enum CONST_BUFFERS
    {
        CB_COMMON_DATA = 0, //per frame update
        CB_LIGHTS =  1, //per frame update
        CB_TERRAIN = 2, //per frame update
        CB_LAST
    };

    struct CB_COMMON_DATA_STRUCT {
        alignas(16) glm::vec3 vViewPos;
        float                 fTime;
        alignas(16) glm::mat4 mViewProj;
    };

/*
    enum CB_DYNAMIC_FIELDS {
        ED_CB_DYNAMIC_F_TIME,
        ED_CB_DYNAMIC_V_VIEW_POS,
        ED_CB_DYNAMIC_M_VIEV_PROJ
    };

    const unsigned int CB_DYNAMIC_OFFSET[] =
    {
        offsetof(CB_DYNAMIC_STRUCT, fTime),
        offsetof(CB_DYNAMIC_STRUCT, vViewPos),
        offsetof(CB_DYNAMIC_STRUCT, mViewProj)
    };
*/

    struct CB_LIGHTS_STRUCT {
        TRANSFORM_COMPONENT   transform0;
        POINT_LIGHT_COMPONENT light0;
    };

/*
    enum CB_LIGHTS_FIELDS {
        ED_CB_LIGHTS_LIGHT0,
    };

    const unsigned int CB_LIGHTS_OFFSET[] =
    {
        offsetof(CB_LIGHTS_STRUCT, light0),
    };
*/

    struct CB_TERRAIN_STRUCT {
        glm::vec2 terrainStartPos;
    };

    const unsigned int CONST_BUFFERS_SIZE[] =
    {
        sizeof(CB_COMMON_DATA_STRUCT),
        sizeof(CB_LIGHTS_STRUCT),
        sizeof(CB_TERRAIN_STRUCT)
    };

    const unsigned int CONST_BUFFERS_SLOT[] =
    {
        0,
        1,
        2
    };
}
