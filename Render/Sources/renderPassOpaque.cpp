#include "renderPassOpaque.h"

#include "commonRenderVariables.h"
#include "ecsCoordinator.h"

#include "Components/rendered.h"
#include "Components/camera.h"
#include "Components/transformation.h"
#include "Events/debug.h"

#include "materialManager.h"
#include "meshManager.h"
#include "renderTargetManager.h"

void RENDER_PASS_OPAQUE::Init()
{
    ECS::pEcsCoordinator->SubscrubeSystemToComponentType<RENDERED_COMPONENT>(this);
    ECS::pEcsCoordinator->SubscrubeSystemToComponentType<MESH_COMPONENT>(this);
}

void RENDER_PASS_OPAQUE::BeginRenderPass()
{
    pDrvInterface->SetRenderTarget(RT_FP16, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    pDrvInterface->SetDepthBuffer(RT_DEPTH_BUFFER, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    pDrvInterface->SetRenderTargetAsShaderResource(RT_SHADOW_MAP, 30);
    pDrvInterface->BeginRenderPass();
}

void RENDER_PASS_OPAQUE::EndRenderPass()
{
    pDrvInterface->EndRenderPass();
}

void RENDER_PASS_OPAQUE::Render()
{
    BeginRenderPass();
    pDrvInterface->SetShader(EFFECT_DATA::SHR_BASE);

    pDrvInterface->SetDepthTestState(true);
    pDrvInterface->SetDepthWriteState(true);
    pDrvInterface->SetDepthComparitionOperation(true);
    pDrvInterface->SetStencilTestState(false);


    EFFECT_DATA::CB_COMMON_DATA_STRUCT dynBufferData;
    dynBufferData.fTime = pDrvInterface->GetCurTime();
    dynBufferData.vViewPos = ECS::pEcsCoordinator->GetComponent<CAMERA_TRANSFORM_COMPONENT>(gameCamera)->position;
    dynBufferData.mViewProj = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(gameCamera)->viewProjMatrix;
    pDrvInterface->FillConstBuffer(EFFECT_DATA::CB_COMMON_DATA, &dynBufferData, EFFECT_DATA::CONST_BUFFERS_SIZE[EFFECT_DATA::CB_COMMON_DATA]);

    EFFECT_DATA::CB_LIGHTS_STRUCT lightBufferData;
    lightBufferData.transform0 = *ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(lights[0]);
    lightBufferData.light0 = *ECS::pEcsCoordinator->GetComponent<POINT_LIGHT_COMPONENT>(lights[0]);
    pDrvInterface->FillConstBuffer(EFFECT_DATA::CB_LIGHTS, &lightBufferData, EFFECT_DATA::CONST_BUFFERS_SIZE[EFFECT_DATA::CB_LIGHTS]);

    for (auto& rendEntity : m_entityList) {
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_COMMON_DATA);
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_LIGHTS);

        glm::mat4x4 worldTransformMatrix(1.f);
        const ROTATE_COMPONENT* rotate = ECS::pEcsCoordinator->GetComponent<ROTATE_COMPONENT>(rendEntity);
        if (rotate) {
            worldTransformMatrix = glm::mat4_cast(rotate->quaternion);
        }

        const TRANSFORM_COMPONENT* transform = ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(rendEntity);
        if (transform) {
            worldTransformMatrix[3].x = transform->position.x;
            worldTransformMatrix[3].y = transform->position.y;
            worldTransformMatrix[3].z = transform->position.z;
        }
        pDrvInterface->FillPushConstantBuffer(&worldTransformMatrix, sizeof(worldTransformMatrix));

        const MATERIAL_COMPONENT* material = ECS::pEcsCoordinator->GetComponent<MATERIAL_COMPONENT>(rendEntity);
        if (material) {
            pDrvInterface->SetTexture(material->pAlbedoTex, 16);
            pDrvInterface->SetTexture(material->pNormalTex, 17);
            pDrvInterface->SetTexture(material->pDisplacementTex, 18);
            pDrvInterface->SetTexture(material->pRoughnessTex, 19);
            pDrvInterface->SetTexture(material->pMetalnessTex, 20);
        } else {
            for (int slot = 16; slot <= 20; slot++) {
                pDrvInterface->SetTexture(nullptr, slot);
            }
        }

        const MESH_COMPONENT* mesh = ECS::pEcsCoordinator->GetComponent<MESH_COMPONENT>(rendEntity);
        pDrvInterface->SetVertexFormat(mesh->pMesh->vertexFormatId);
        pDrvInterface->SetVertexBuffer(mesh->pMesh->vertexBuffer, 0);
        pDrvInterface->SetIndexBuffer(mesh->pMesh->indexBuffer, 0);
        pDrvInterface->DrawIndexed(mesh->pMesh->numOfIndexes);
    }
    EndRenderPass();
}
