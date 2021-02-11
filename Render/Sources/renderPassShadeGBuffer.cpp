#include "renderPassShadeGBuffer.h"

#include "commonRenderVariables.h"
#include "ecsCoordinator.h"

#include "geometry.h"

#include "Components/rendered.h"
#include "Components/camera.h"
#include "Components/transformation.h"
#include "Events/debug.h"

#include "resourceSystem.h"
#include "renderTargetManager.h"

void RENDER_PASS_SHADE_GBUFFER::Init()
{
}

void RENDER_PASS_SHADE_GBUFFER::BeginRenderPass()
{
    pRenderTargetManager->SetTextureAsRenderTarget(RT_FP16, 0, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);
    pRenderTargetManager->SetTextureAsSRV(RT_GBUFFER_ALBEDO, 20);
    pRenderTargetManager->SetTextureAsSRV(RT_GBUFFER_NORMAL, 21);
    pRenderTargetManager->SetTextureAsSRV(RT_GBUFFER_METALL_ROUGHNESS, 22);
    pRenderTargetManager->SetTextureAsSRV(RT_GBUFFER_WORLD_POS, 23);
    //pRenderTargetManager->SetTextureAsSRV(RT_DEPTH_BUFFER, 24);
    pRenderTargetManager->SetTextureAsSRV(RT_SHADOW_MAP, 24);
    pDrvInterface->BeginRenderPass();
}

void RENDER_PASS_SHADE_GBUFFER::EndRenderPass()
{
    pDrvInterface->EndRenderPass();
    pRenderTargetManager->ReturnAllRenderTargetsToPool();
}

void RENDER_PASS_SHADE_GBUFFER::Render()
{
    BeginRenderPass();

    const glm::vec4 skyColor = { 0.1f, 0.6f, 0.9f, 0.f };
    pDrvInterface->ClearBackBuffer(skyColor);

    pDrvInterface->SetShader(EFFECT_DATA::SHR_SHADE_GBUFFER);

    pDrvInterface->SetDepthTestState(true);
    pDrvInterface->SetDepthWriteState(true);
    pDrvInterface->SetDepthComparitionOperation(true);
    pDrvInterface->SetStencilTestState(false);

    EFFECT_DATA::CB_COMMON_DATA_STRUCT dynBufferData;
    dynBufferData.fTime = 0.f;
    dynBufferData.vViewPos = ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(gameCamera)->position;
    dynBufferData.mViewProj = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(gameCamera)->viewProjMatrix;
    pDrvInterface->FillConstBuffer(EFFECT_DATA::CB_COMMON_DATA, &dynBufferData, EFFECT_DATA::CONST_BUFFERS_SIZE[EFFECT_DATA::CB_COMMON_DATA]);

    EFFECT_DATA::CB_LIGHTS_STRUCT lightBufferData;
    lightBufferData.dirLight = *ECS::pEcsCoordinator->GetComponent<DIRECTIONAL_LIGHT_COMPONENT>(directionalLight);
    lightBufferData.pointLight0Transform = *ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(pointLights[0]);
    lightBufferData.pointLight0 = *ECS::pEcsCoordinator->GetComponent<POINT_LIGHT_COMPONENT>(pointLights[0]);
    lightBufferData.ambientColor = COMMON_AMBIENT;
    //tmp
    lightBufferData.dirLightViewProj = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(directionalLight)->viewProjMatrix;
    lightBufferData.pointLight0ViewProj = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(pointLights[0])->viewProjMatrix;
    pDrvInterface->FillConstBuffer(EFFECT_DATA::CB_LIGHTS, &lightBufferData, EFFECT_DATA::CONST_BUFFERS_SIZE[EFFECT_DATA::CB_LIGHTS]);

    pRenderTargetManager->ReturnRenderTarget(RT_SHADOW_MAP);

    pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_COMMON_DATA);
    pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_LIGHTS);

    pDrvInterface->SetVertexFormat(EMPTY_VERTEX::formatId);
    pDrvInterface->DrawFullscreen();

    EndRenderPass();
}
