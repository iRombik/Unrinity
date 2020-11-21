#include "renderPassOpaque.h"

#include "commonRenderVariables.h"
#include "ecsCoordinator.h"

#include "Components/rendered.h"
#include "Components/camera.h"
#include "Components/transformation.h"
#include "Events/debug.h"

#include "resourceSystem.h"
#include "renderTargetManager.h"

void RENDER_PASS_OPAQUE::Init()
{
    ECS::pEcsCoordinator->SubscrubeSystemToComponentType<VISIBLE_COMPONENT>(this);
    ECS::pEcsCoordinator->SubscrubeSystemToComponentType<MESH_PRIMITIVE>(this);
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

    for (auto rendEntity : m_entityList) {
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_COMMON_DATA);
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_LIGHTS);

        glm::mat4x4 worldTransformMatrix(1.f);
//         const ROTATE_COMPONENT* rotate = ECS::pEcsCoordinator->GetComponent<ROTATE_COMPONENT>(rendEntity);
//         if (rotate) {
//             worldTransformMatrix = glm::mat4_cast(rotate->quaternion);
//         }
// 
//         const TRANSFORM_COMPONENT* transform = ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(rendEntity);
//         if (transform) {
//             worldTransformMatrix = glm::translate(worldTransformMatrix, transform->position);
//         }

        const MESH_PRIMITIVE* pMeshPrimitive = ECS::pEcsCoordinator->GetComponent<MESH_PRIMITIVE>(rendEntity);
        const NODE_COMPONENT* pNode = pMeshPrimitive->pParentHolder->pParentsNodes[0];
        //worldTransformMatrix = pNode->matrix;
        worldTransformMatrix = glm::mat4_cast(pNode->rotation);
        worldTransformMatrix = glm::translate(worldTransformMatrix, pNode->translation);
        pDrvInterface->FillPushConstantBuffer(&worldTransformMatrix, sizeof(worldTransformMatrix));

        const MATERIAL_COMPONENT* material = pMeshPrimitive->pMaterial;
        if (material) {
            pDrvInterface->SetTexture(material->pAlbedoTex, 16);
            pDrvInterface->SetTexture(material->pNormalTex, 17);
            pDrvInterface->SetTexture(material->pDisplacementTex, 18);
            pDrvInterface->SetTexture(material->pMetalRoughnessTex, 19);
        } else {
            for (int slot = 16; slot <= 19; slot++) {
                pDrvInterface->SetTexture(nullptr, slot);
            }
        }

        const VULKAN_MESH* pMesh = pMeshPrimitive->pMesh;
        pDrvInterface->SetVertexFormat(pMesh->vertexFormatId);
        pDrvInterface->SetVertexBuffer(pMesh->vertexBuffer, 0);
        pDrvInterface->SetIndexBuffer(pMesh->indexBuffer, 0);
        if (pMesh->numOfIndexes == 0) {
            pDrvInterface->Draw(pMesh->numOfVertexes);
        } else {
            pDrvInterface->DrawIndexed(pMesh->numOfIndexes);
        }
    }
    EndRenderPass();
}
