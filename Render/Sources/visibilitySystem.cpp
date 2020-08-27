#include "visibilitySystem.h"

#include "Components/rendered.h"

bool VISIBILITY_SYSTEM::Init()
{
    ECS::pEcsCoordinator->SubscrubeSystemToComponentType<RENDERED_COMPONENT>(this);
    return true;
}

void VISIBILITY_SYSTEM::Update()
{
    for (auto& rendEntity : m_entityList) {
        ECS::pEcsCoordinator->AddComponentToEntity(rendEntity, VISIBLE_COMPONENT());
    }
}
