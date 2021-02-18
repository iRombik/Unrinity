#pragma once
#include "vulkanDriver.h"
#include "ecsCoordinator.h"

class RENDER_PASS_BLEND_SSAO : public ECS::SYSTEM<RENDER_PASS_BLEND_SSAO> {
public:
    void Init();
    void Render();
private:
    void BeginRenderPass();
    void EndRenderPass();
};