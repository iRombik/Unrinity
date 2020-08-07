#pragma once

#include "ecsCoordinator.h"
#include "vulkanDriver.h"

class RENDER_PASS_RESOLVE : public ECS::SYSTEM<RENDER_PASS_RESOLVE> {
public:
    void Init();
    void Render();
private:
    void BeginRenderPass();
    void EndRenderPass();
private:
    VkRenderPass m_renderPass;
};