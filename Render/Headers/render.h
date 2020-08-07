#pragma once
#include <vector>
#include "ecsCoordinator.h"

class RENDER_SYSTEM : public ECS::SYSTEM<RENDER_SYSTEM> {
public:
    bool Init();
    void Render();
    void Term();
};
