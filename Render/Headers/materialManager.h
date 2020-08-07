#pragma once
#include "vulkanDriver.h"
#include "systemManager.h"
#include "componentManager.h"

struct MATERIAL_COMPONENT : ECS::COMPONENT<MATERIAL_COMPONENT> 
{
    const VULKAN_TEXTURE* pAlbedoTex;
    const VULKAN_TEXTURE* pNormalTex;
    const VULKAN_TEXTURE* pDisplacementTex;
    const VULKAN_TEXTURE* pRoughnessTex;
    const VULKAN_TEXTURE* pMetalnessTex;
};

struct CUSTOM_MATERIAL_COMPONENT : ECS::COMPONENT<CUSTOM_MATERIAL_COMPONENT>
{
    float     customRoughness;
    glm::vec3 customMetalness;
};

class MATERIAL_MANAGER
{
    void CreateMaterial();
    void SetupMaterial();
};
