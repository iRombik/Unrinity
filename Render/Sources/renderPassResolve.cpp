#include "renderPassResolve.h"

#include "ecsCoordinator.h"
#include "renderTargetManager.h"
#include "geometry.h"

void RENDER_PASS_RESOLVE::Init()
{
}

void RENDER_PASS_RESOLVE::Render()
{
    BeginRenderPass();
    Draw();
    EndRenderPass();
}

void RENDER_PASS_RESOLVE::BeginRenderPass()
{
    pRenderTargetManager->SetTextureAsRenderTarget(RT_BACK_BUFFER, 0, 
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);
    pRenderTargetManager->SetTextureAsSRV(RT_FP16, 20);
    pDrvInterface->BeginRenderPass();
}

void RENDER_PASS_RESOLVE::Draw()
{
    pDrvInterface->SetShader(EFFECT_DATA::SHR_FULLSCREEN);
    pDrvInterface->SetVertexFormat(EMPTY_VERTEX::formatId);
    pDrvInterface->DrawFullscreen();
}

void RENDER_PASS_RESOLVE::EndRenderPass()
{
    pDrvInterface->EndRenderPass();
    pRenderTargetManager->ReturnAllRenderTargetsToPool();
}
