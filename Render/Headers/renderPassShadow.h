#pragma once

#include "vulkanDriver.h"
#include "ecsCoordinator.h"

class RENDER_PASS_SHADOW : public ECS::SYSTEM<RENDER_PASS_SHADOW> {
public:
    void Init();
    void Update();
    void Render();
private:
    void BeginRenderPass();
    void EndRenderPass();
private:
    VkRenderPass m_renderPass;
    VkFramebuffer m_frameBuffer;
};

