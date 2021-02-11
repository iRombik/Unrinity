#pragma once
#include "vulkanDriver.h"
#include "ecsCoordinator.h"

class RENDER_PASS_SHADE_GBUFFER : public ECS::SYSTEM<RENDER_PASS_SHADE_GBUFFER> {
public:
    void Init();
    void Render();
private:
    void BeginRenderPass();
    void EndRenderPass();
private:
    VkRenderPass m_renderPass;
    VkFramebuffer m_frameBuffer;
};
