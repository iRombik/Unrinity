#pragma once
#include "vulkanDriver.h"
#include "ecsCoordinator.h"

class RENDER_PASS_OPAQUE : public ECS::SYSTEM<RENDER_PASS_OPAQUE> {
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
