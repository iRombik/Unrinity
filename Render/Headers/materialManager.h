#pragma once
#include "vulkanDriver.h"
#include "systemManager.h"
#include "componentManager.h"

struct VULKAN_TEXTURE;

struct MATERIAL_COMPONENT : public ECS::COMPONENT<MATERIAL_COMPONENT>
{
    MATERIAL_COMPONENT() : pAlbedoTex(nullptr), pNormalTex(nullptr), pDisplacementTex(nullptr), pMetalRoughnessTex(nullptr), pEmissiveTex(nullptr),
    isDoubleSided(false), alphaMode(ALPHA_MODE::ALPHA_OPAQUE), alphaCutFactor(1.f), baseColor(0.f) {}

    enum class ALPHA_MODE { ALPHA_OPAQUE, ALPHA_BLEND, ALPHA_MASK };

    bool isDoubleSided;
    ALPHA_MODE alphaMode;
    float alphaCutFactor;
    glm::fvec4 baseColor;

    const VULKAN_TEXTURE* pAlbedoTex;
    const VULKAN_TEXTURE* pNormalTex;
    const VULKAN_TEXTURE* pMetalRoughnessTex;
    //const VULKAN_TEXTURE* pRoughnessTex;
    //const VULKAN_TEXTURE* pMetalnessTex;
    const VULKAN_TEXTURE* pDisplacementTex;
    const VULKAN_TEXTURE* pEmissiveTex;
};
