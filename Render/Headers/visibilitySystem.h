#pragma once
#include "ecsCoordinator.h"

class VISIBILITY_SYSTEM : public ECS::SYSTEM<VISIBILITY_SYSTEM>
{
public:
    bool Init();
    void Update();
};
