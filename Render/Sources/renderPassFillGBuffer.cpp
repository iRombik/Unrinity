#include "renderPassFillGBuffer.h"

#include "commonRenderVariables.h"
#include "ecsCoordinator.h"

#include "Components/rendered.h"
#include "Components/camera.h"
#include "Components/transformation.h"
#include "Events/debug.h"

#include "resourceSystem.h"
#include "renderTargetManager.h"

void RENDER_PASS_FILL_GBUFFER::Init()
{
    ECS::pEcsCoordinator->SubscrubeSystemToComponentType<VISIBLE_COMPONENT>(this);
    ECS::pEcsCoordinator->SubscrubeSystemToComponentType<MESH_PRIMITIVE>(this);
}

void RENDER_PASS_FILL_GBUFFER::BeginRenderPass()
{
    pRenderTargetManager->SetTextureAsRenderTarget(RT_GBUFFER_ALBEDO, 0, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    pRenderTargetManager->SetTextureAsRenderTarget(RT_GBUFFER_NORMAL, 1, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    pRenderTargetManager->SetTextureAsRenderTarget(RT_GBUFFER_METALL_ROUGHNESS, 2, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    pRenderTargetManager->SetTextureAsRenderTarget(RT_GBUFFER_WORLD_POS, 3, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    pRenderTargetManager->SetTextureAsDepthBuffer(RT_DEPTH_BUFFER, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    pDrvInterface->BeginRenderPass();
}

void RENDER_PASS_FILL_GBUFFER::EndRenderPass()
{
    pDrvInterface->EndRenderPass();
    pRenderTargetManager->ReturnAllRenderTargetsToPool();
}

void RENDER_PASS_FILL_GBUFFER::Render()
{
    BeginRenderPass();

    const glm::vec4 skyColor = { 0.1f, 0.6f, 0.9f, 0.f };
    pDrvInterface->ClearBackBuffer(skyColor);

    pDrvInterface->SetShader(EFFECT_DATA::SHR_FILL_GBUFFER);

    pDrvInterface->SetDepthTestState(true);
    pDrvInterface->SetDepthWriteState(true);
    pDrvInterface->SetDepthComparitionOperation(true);
    pDrvInterface->SetStencilTestState(false);

    EFFECT_DATA::CB_COMMON_DATA_STRUCT dynBufferData;
    dynBufferData.fTime = 0.f;
    dynBufferData.vViewPos = ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(gameCamera)->position;
    dynBufferData.mViewProj = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(gameCamera)->viewProjMatrix;
    dynBufferData.mProj = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(gameCamera)->projMatrix;
    dynBufferData.mProjInv = glm::inverse(ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(gameCamera)->projMatrix);
    dynBufferData.mView = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(gameCamera)->viewMatrix;
    pDrvInterface->FillConstBuffer(EFFECT_DATA::CB_COMMON_DATA, &dynBufferData, EFFECT_DATA::CONST_BUFFERS_SIZE[EFFECT_DATA::CB_COMMON_DATA]);

    EFFECT_DATA::CB_DEBUG_STRUCT debugBufferData;
    debugBufferData.drawMode = gDebugVariables.drawMode;
    pDrvInterface->FillConstBuffer(EFFECT_DATA::CB_DEBUG, &debugBufferData, EFFECT_DATA::CONST_BUFFERS_SIZE[EFFECT_DATA::CB_DEBUG]);

    for (auto rendEntity : m_entityList) {
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_COMMON_DATA);
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_DEBUG);

        glm::mat4x4 worldTransformMatrix(1.f);

        const MESH_PRIMITIVE* pMeshPrimitive = ECS::pEcsCoordinator->GetComponent<MESH_PRIMITIVE>(rendEntity);
        //         const NODE_COMPONENT* pNode = pMeshPrimitive->pParentHolder->pParentsNodes[0];
        //         worldTransformMatrix = glm::mat4_cast(pNode->rotation);
        //         worldTransformMatrix = glm::translate(worldTransformMatrix, pNode->translation);
        pDrvInterface->FillPushConstantBuffer(&worldTransformMatrix, sizeof(worldTransformMatrix));

        const MATERIAL_COMPONENT* material = pMeshPrimitive->pMaterial;
        ASSERT(material);

        pDrvInterface->SetTexture(material->pAlbedoTex, 20);
        pDrvInterface->SetTexture(material->pNormalTex, 21);
        pDrvInterface->SetTexture(material->pMetalRoughnessTex, 22);
        //pDrvInterface->SetTexture(material->pDisplacementTex, 23);

        const VULKAN_MESH* pMesh = pMeshPrimitive->pMesh;
        pDrvInterface->SetVertexFormat(pMesh->vertexFormatId);
        pDrvInterface->SetVertexBuffer(pMesh->vertexBuffer, 0);
        if (pMesh->numOfIndexes == 0) {
            pDrvInterface->Draw(pMesh->numOfVertexes);
        } else {
            pDrvInterface->SetIndexBuffer(pMesh->indexBuffer, 0);
            pDrvInterface->DrawIndexed(pMesh->numOfIndexes);
        }
    }
    EndRenderPass();
}
