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

    pDrvInterface->SetShader(EFFECT_DATA::SHR_FULLSCREEN);
    pDrvInterface->SetVertexFormat(EMPTY_VERTEX::formatId);

    pDrvInterface->DrawFullscreen();

    EndRenderPass();
}

void RENDER_PASS_RESOLVE::BeginRenderPass()
{
    pDrvInterface->SetRenderTarget(RT_BACK_BUFFER, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);
    pDrvInterface->SetRenderTargetAsShaderResource(RT_FP16, 16);
    pDrvInterface->BeginRenderPass();
}

void RENDER_PASS_RESOLVE::EndRenderPass()
{
    pDrvInterface->EndRenderPass();
}
