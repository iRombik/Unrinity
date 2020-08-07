#include "renderPassShadow.h"

#include "commonRenderVariables.h"
#include "ecsCoordinator.h"

#include "Components/rendered.h"
#include "Components/camera.h"
#include "Components/transformation.h"

#include "meshManager.h"
#include "renderTargetManager.h"

void RENDER_PASS_SHADOW::Init()
{
    ECS::pEcsCoordinator->SubscrubeSystemToComponentType<RENDERED_COMPONENT>(this);
    ECS::pEcsCoordinator->SubscrubeSystemToComponentType<MESH_COMPONENT>(this);
}

void RENDER_PASS_SHADOW::Render()
{
    BeginRenderPass();

    pDrvInterface->SetShader(EFFECT_DATA::SHR_SHADOW);

    pDrvInterface->SetDepthTestState(true);
    pDrvInterface->SetDepthWriteState(true);
    pDrvInterface->SetDepthComparitionOperation(true);
    pDrvInterface->SetStencilTestState(false);


    EFFECT_DATA::CB_COMMON_DATA_STRUCT dynBufferData;
    dynBufferData.fTime = pDrvInterface->GetCurTime();
    dynBufferData.vViewPos = ECS::pEcsCoordinator->GetComponent<CAMERA_TRANSFORM_COMPONENT>(gameCamera)->position;
    dynBufferData.mViewProj = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(gameCamera)->viewProjMatrix;
    pDrvInterface->FillConstBuffer(EFFECT_DATA::CB_COMMON_DATA, &dynBufferData, EFFECT_DATA::CONST_BUFFERS_SIZE[EFFECT_DATA::CB_COMMON_DATA]);

    for (auto& rendEntity : m_entityList) {
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_COMMON_DATA);

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

        const MESH_COMPONENT* mesh = ECS::pEcsCoordinator->GetComponent<MESH_COMPONENT>(rendEntity);
        pDrvInterface->SetVertexFormat(mesh->pMesh->vertexFormatId);
        pDrvInterface->SetVertexBuffer(mesh->pMesh->vertexBuffer, 0);
        pDrvInterface->SetIndexBuffer(mesh->pMesh->indexBuffer, 0);
        pDrvInterface->DrawIndexed(mesh->pMesh->numOfIndexes);
    }

    EndRenderPass();
}

void RENDER_PASS_SHADOW::BeginRenderPass()
{
    pDrvInterface->SetDepthBuffer(RT_SHADOW_MAP, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    pDrvInterface->BeginRenderPass();
}

void RENDER_PASS_SHADOW::EndRenderPass()
{
    pDrvInterface->EndRenderPass();
}
