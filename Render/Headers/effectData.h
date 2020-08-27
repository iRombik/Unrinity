#pragma once
#define  GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
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
        SAMPLER_REPEAT_LINEAR_ANISO = 0,
        SAMPLER_REPEAT_LINEAR       = 1,
        SAMPLER_CLAMP_LINEAR        = 2,
        SAMPLER_CLAMP_POINT         = 3,
        SAMPLER_CLAMP_POINT_CMP     = 4,
        SAMPLER_LAST
    };
}

namespace EFFECT_DATA 
{
    enum CONST_BUFFERS
    {
        CB_COMMON_DATA = 0, //per frame update
        CB_LIGHTS =  1, //per frame update
        CB_TERRAIN = 2, //per frame update
        CB_MATERIAL = 3, //per draw update
        CB_LAST
    };

    struct CB_COMMON_DATA_STRUCT {
        alignas(16) glm::vec3 vViewPos;
        float                 fTime;
        alignas(16) glm::mat4 mViewProj;
    };

    struct CB_LIGHTS_STRUCT {
        TRANSFORM_COMPONENT         dirLightTransform;
        DIRECTIONAL_LIGHT_COMPONENT dirLight;

        alignas(16) TRANSFORM_COMPONENT pointLight0Transform;
        POINT_LIGHT_COMPONENT           pointLight0;
        
        glm::mat4x4 dirLightViewProj;
        glm::mat4x4 pointLight0ViewProj;

        glm::vec3 ambientColor;
    };

    struct CB_TERRAIN_STRUCT {
        glm::vec2 terrainStartPos;
    };

    struct CB_MATERIAL_STRUCT {
        glm::vec3 metalness;
        float     roughness;
    };

    const uint32_t CONST_BUFFERS_SIZE[] =
    {
        sizeof(CB_COMMON_DATA_STRUCT),
        sizeof(CB_LIGHTS_STRUCT),
        sizeof(CB_TERRAIN_STRUCT),
        sizeof(CB_MATERIAL_STRUCT)
    };

    const unsigned int CONST_BUFFERS_SLOT[] =
    {
        0,
        1,
        2,
        3
    };
}
