#pragma once
#define  GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include "glm/glm.hpp"
#include "Components/transformation.h"
#include "Components/lightSource.h"

namespace EFFECT_DATA {
    enum SHADER_ID {
        SHR_FULLSCREEN = 0,
        SHR_FILL_GBUFFER = 1,
        SHR_SHADE_GBUFFER = 2,
        SHR_SHADOW = 3,
        SHR_SSAO = 4,
        SHR_SSAO_BLEND = 5,
        SHR_TERRAIN = 6,
        SHR_UI = 7,
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
        SAMPLER_REPEAT_POINT        = 0,
        SAMPLER_REPEAT_LINEAR       = 1,
        SAMPLER_REPEAT_LINEAR_ANISO = 2,
        SAMPLER_CLAMP_POINT         = 3,
        SAMPLER_CLAMP_LINEAR        = 4,
        SAMPLER_CLAMP_POINT_CMP     = 5,
        SAMPLER_CLAMP_LINEAR_CMP    = 6,
        SAMPLER_LAST
    };
}

namespace EFFECT_DATA 
{
    enum CONST_BUFFERS
    {
        CB_COMMON_DATA = 0, //per frame update
        CB_LIGHTS =  1, //per frame update
        CB_MATERIAL = 2, //per draw update
        CB_TERRAIN = 3, //per frame update
        CB_UI,
        CB_CUSTOM,
        CB_DEBUG,
        CB_LAST
    };

    struct CB_COMMON_DATA_STRUCT {
        alignas(16) glm::vec3 vViewPos;
        float                 fTime;
        alignas(16) glm::mat4 mViewProj;
        alignas(16) glm::mat4 mProj;
        alignas(16) glm::mat4 mProjInv;
        alignas(16) glm::mat4 mView;
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

    struct CB_UI_STRUCT
    {
        glm::vec2 scale;
        glm::vec2 translate;
    };
    
    struct CB_CUSTOM_STRUCT {
        glm::vec4 cb0;
        glm::vec4 cb1;
        glm::vec4 cb2;
        glm::vec4 cb3;
    };

    struct CB_DEBUG_STRUCT {
        int drawMode;
    };

    const uint32_t CONST_BUFFERS_SIZE[] =
    {
        sizeof(CB_COMMON_DATA_STRUCT),
        sizeof(CB_LIGHTS_STRUCT),
        sizeof(CB_MATERIAL_STRUCT),
        sizeof(CB_TERRAIN_STRUCT),
        sizeof(CB_UI_STRUCT),
        sizeof(CB_CUSTOM_STRUCT),
        sizeof(CB_DEBUG_STRUCT),
    };

    const unsigned int CONST_BUFFERS_SLOT[] =
    {
        0,
        1,
        2,
        5,
        6,
        4,
        15,
    };
}
