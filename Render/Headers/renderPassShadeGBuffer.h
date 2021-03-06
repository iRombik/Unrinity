#pragma once
#include "ecsCoordinator.h"

class RENDER_PASS_SHADE_GBUFFER : public ECS::SYSTEM<RENDER_PASS_SHADE_GBUFFER> {
public:
    void Init();
    void Render();
private:
    void BeginRenderPass();
    void EndRenderPass();
};
