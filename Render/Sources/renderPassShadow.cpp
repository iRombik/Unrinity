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

void RENDER_PASS_SHADOW::Update()
{
    const glm::mat4 clip(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    {
        const TRANSFORM_COMPONENT* transform = ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(pointLights[0]);
        CAMERA_COMPONENT* cameraComponent = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(pointLights[0]);
        cameraComponent->viewMatrix = glm::lookAt(transform->position, glm::vec3(0.f, 0.f, 0.f), UP_VECTOR);
        cameraComponent->projMatrix = glm::perspective(glm::radians(45.f), 1.f, 0.1f, 100.f);
        cameraComponent->viewProjMatrix = clip * cameraComponent->projMatrix * cameraComponent->viewMatrix;
    }

    {
        const TRANSFORM_COMPONENT* transform = ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(directionalLight);
        CAMERA_COMPONENT* cameraComponent = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(directionalLight);
        cameraComponent->viewMatrix = glm::lookAt(transform->position, glm::vec3(0.1f, 0.f, 0.1f), UP_VECTOR);
        cameraComponent->projMatrix = glm::ortho(-100.f, 100.f, -100.f, 100.f, 0.1f, 100.f);
        cameraComponent->viewProjMatrix = clip * cameraComponent->projMatrix * cameraComponent->viewMatrix;
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

    EFFECT_DATA::CB_LIGHTS_STRUCT lightBufferData;
//     lightBufferData.dirLightTransform = *ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(directionalLight);
//     lightBufferData.pointLight0Transform = *ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(pointLights[0]);

    lightBufferData.dirLightViewProj = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(directionalLight)->viewProjMatrix;
    lightBufferData.pointLight0ViewProj = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(pointLights[0])->viewProjMatrix;

    pDrvInterface->FillConstBuffer(EFFECT_DATA::CB_LIGHTS, &lightBufferData, EFFECT_DATA::CONST_BUFFERS_SIZE[EFFECT_DATA::CB_LIGHTS]);

    for (auto& rendEntity : m_entityList) {
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_LIGHTS);

        glm::mat4x4 worldTransformMatrix(1.f);
        const ROTATE_COMPONENT* rotate = ECS::pEcsCoordinator->GetComponent<ROTATE_COMPONENT>(rendEntity);
        if (rotate) {
            worldTransformMatrix = glm::mat4_cast(rotate->quaternion);
        }

        const TRANSFORM_COMPONENT* transform = ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(rendEntity);
        if (transform) {
            worldTransformMatrix = glm::translate(worldTransformMatrix, transform->position);
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
