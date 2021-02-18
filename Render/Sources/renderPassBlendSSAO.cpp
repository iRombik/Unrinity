#include "renderPassBlendSSAO.h"

#include "ecsCoordinator.h"

#include "geometry.h"

#include "commonRenderVariables.h"
#include "renderTargetManager.h"

void RENDER_PASS_BLEND_SSAO::Init()
{
}

void RENDER_PASS_BLEND_SSAO::Render()
{
    BeginRenderPass();
    
    pDrvInterface->SetShader(EFFECT_DATA::SHR_SSAO_BLEND);
    pDrvInterface->SetVertexFormat(EMPTY_VERTEX::formatId);
    pDrvInterface->DrawFullscreen();

    EndRenderPass();
}

void RENDER_PASS_BLEND_SSAO::BeginRenderPass()
{
    pRenderTargetManager->SetTextureAsRenderTarget(RT_SSAO_MASK_BLENDED, 0, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);
    pRenderTargetManager->SetTextureAsSRV(RT_SSAO_MASK, 30);

    pDrvInterface->BeginRenderPass();
}

void RENDER_PASS_BLEND_SSAO::EndRenderPass()
{
    pDrvInterface->EndRenderPass();
    pRenderTargetManager->ReturnAllRenderTargetsToPool();
}
