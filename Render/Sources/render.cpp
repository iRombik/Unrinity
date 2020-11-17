#include <glm/gtc/quaternion.hpp>

#include "commonRenderVariables.h"
#include "terrain.h"
#include "gui.h"
#include "effectData.h"
#include "textureManager.h"
#include "materialManager.h"
#include "meshManager.h"
#include "render.h"
#include "resourceSystem.h"
#include "renderTargetManager.h"
#include "support.h"
#include "vulkanDriver.h"

#include "ecsCoordinator.h"
#include "Components/camera.h"
#include "Components/transformation.h"
#include "Events/debug.h"

#include "visibilitySystem.h"
#include "renderPassOpaque.h"
#include "renderPassResolve.h"
#include "renderPassShadow.h"

glm::vec3 COMMON_AMBIENT = glm::vec3(0.2f);
glm::vec2 DEPTH_BIAS_PARAMS = glm::vec2(1.25f, 1.65f);

ECS::ENTITY_TYPE gameCamera = ECS::INVALID_ENTITY_ID;
std::vector<ECS::ENTITY_TYPE> pointLights;
ECS::ENTITY_TYPE directionalLight = ECS::INVALID_ENTITY_ID;

bool RENDER_SYSTEM::Init()
{
    pDrvInterface.reset(new VULKAN_DRIVER_INTERFACE());
    pRenderTargetManager.reset(new RENDER_TARGET_MANAGER());

    bool isDriverInited = pDrvInterface->Init();
    if (!isDriverInited) {
        ERROR_MSG("Driver creation failed!");
        return false;
    }

    if (pDrvInterface->InitPipelineState() != VK_SUCCESS) {
        return false;
    }

    pRenderTargetManager->Init(pDrvInterface->GetBackBufferWidth(), pDrvInterface->GetBackBufferHeight());

    ECS::pEcsCoordinator->CreateSystem<VISIBILITY_SYSTEM>()->Init();
    ECS::pEcsCoordinator->CreateSystem<RENDER_PASS_OPAQUE>()->Init();
    ECS::pEcsCoordinator->CreateSystem<RENDER_PASS_RESOLVE>()->Init();
    ECS::pEcsCoordinator->CreateSystem<RENDER_PASS_SHADOW>()->Init();

    return true;
}

void RENDER_SYSTEM::Update()
{
    ECS::pEcsCoordinator->GetSystem <VISIBILITY_SYSTEM>()->Update();
    ECS::pEcsCoordinator->GetSystem <RENDER_PASS_SHADOW>()->Update();
}

void RENDER_SYSTEM::Render()
{
    pDrvInterface->StartFrame(0.f);

    ECS::pEcsCoordinator->GetSystem <RENDER_PASS_SHADOW>()->Render();
    ECS::pEcsCoordinator->GetSystem<RENDER_PASS_OPAQUE>()->Render();
    ECS::pEcsCoordinator->GetSystem<TERRAIN_SYSTEM>()->Render();
    ECS::pEcsCoordinator->GetSystem<RENDER_PASS_RESOLVE>()->Render();
    ECS::pEcsCoordinator->GetSystem<GUI_SYSTEM>()->Render();

    pDrvInterface->EndFrame(0.f);
}

void RENDER_SYSTEM::Term()
{
    pResourceSystem->Term();
    pDrvInterface->Term();
    pResourceSystem.release();
    pDrvInterface.release();
}
