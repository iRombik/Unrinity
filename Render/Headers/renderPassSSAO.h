#pragma once
#include "vulkanDriver.h"
#include "ecsCoordinator.h"

class RENDER_PASS_SSAO : public ECS::SYSTEM<RENDER_PASS_SSAO> {
public:
    void Init();
    void Render();
private:
    void BeginRenderPass();
    void EndRenderPass();

    void CreateKernelTexture();
    void CreateNoiseTexture();
private:
    VULKAN_TEXTURE m_kernelTexture;
    VULKAN_TEXTURE m_noiseTexture;
};