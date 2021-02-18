#include "renderPassShadow.h"

#include "commonRenderVariables.h"
#include "ecsCoordinator.h"

#include "Components/rendered.h"
#include "Components/camera.h"
#include "Components/transformation.h"

#include "meshManager.h"
#include "renderTargetManager.h"
#include "resourceSystem.h"

void RENDER_PASS_SHADOW::Init()
{
    ECS::pEcsCoordinator->SubscrubeSystemToComponentType<RENDERED_COMPONENT>(this);
    ECS::pEcsCoordinator->SubscrubeSystemToComponentType<MESH_PRIMITIVE>(this);
}

void RENDER_PASS_SHADOW::Update()
{
    const glm::mat4 clip(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    {
        const TRANSFORM_COMPONENT* transform = ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(directionalLight);
        CAMERA_COMPONENT* cameraComponent = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(directionalLight);
        cameraComponent->viewMatrix = glm::lookAt(transform->position, glm::vec3(-35.f, 0.f, 0.1f), UP_VECTOR);
        const float width = 300.f;
        const float height = 300.f;
        const float nearPlane = 100.f;
        const float farPlane = 3000.f;
        cameraComponent->projMatrix = glm::ortho(-width, width, -height, height, nearPlane, farPlane);
        cameraComponent->viewProjMatrix = clip * cameraComponent->projMatrix * cameraComponent->viewMatrix;

        DIRECTIONAL_LIGHT_COMPONENT* dirLightComponent = ECS::pEcsCoordinator->GetComponent<DIRECTIONAL_LIGHT_COMPONENT>(directionalLight);
        dirLightComponent->direction = glm::vec4(0.f, 0.f, 1.f, 0.f) * cameraComponent->viewMatrix;
    }
}

void RENDER_PASS_SHADOW::Render()
{
    BeginRenderPass();

    pDrvInterface->SetShader(EFFECT_DATA::SHR_SHADOW);

    pDrvInterface->SetDepthTestState(true);
    pDrvInterface->SetDepthWriteState(true);
    pDrvInterface->SetDepthComparitionOperation(true);
    pDrvInterface->SetStencilTestState(false);
    pDrvInterface->SetDynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS, true);
    pDrvInterface->SetDepthBiasParams(DEPTH_BIAS_PARAMS.x, DEPTH_BIAS_PARAMS.y);

    EFFECT_DATA::CB_LIGHTS_STRUCT lightBufferData;
    lightBufferData.dirLightViewProj = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(directionalLight)->viewProjMatrix;
    lightBufferData.pointLight0ViewProj = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(pointLights[0])->viewProjMatrix;

    pDrvInterface->FillConstBuffer(EFFECT_DATA::CB_LIGHTS, &lightBufferData, EFFECT_DATA::CONST_BUFFERS_SIZE[EFFECT_DATA::CB_LIGHTS]);

    for (auto& rendEntity : m_entityList) 
    {
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_LIGHTS);

        glm::mat4x4 worldTransformMatrix(1.f);

        const MESH_PRIMITIVE* pMeshPrimitive = ECS::pEcsCoordinator->GetComponent<MESH_PRIMITIVE>(rendEntity);
//         const NODE_COMPONENT* pNode = pMeshPrimitive->pParentHolder->pParentsNodes[0];
//         worldTransformMatrix = glm::mat4_cast(pNode->rotation);
//         worldTransformMatrix = glm::translate(worldTransformMatrix, pNode->translation);
        pDrvInterface->FillPushConstantBuffer(&worldTransformMatrix, sizeof(worldTransformMatrix));

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

void RENDER_PASS_SHADOW::BeginRenderPass()
{
    pRenderTargetManager->SetTextureAsDepthBuffer(RT_SHADOW_MAP, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    pDrvInterface->BeginRenderPass();
}

void RENDER_PASS_SHADOW::EndRenderPass()
{
    pDrvInterface->EndRenderPass();
    pRenderTargetManager->ReturnAllRenderTargetsToPool();
}
