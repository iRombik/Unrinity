#include "renderPassSSAO.h"

#include "ecsCoordinator.h"

#include "geometry.h"

#include "commonRenderVariables.h"
#include "renderTargetManager.h"
#include "randomGenerator.h"

#include "vulkanDriver.h"

void RENDER_PASS_SSAO::Init()
{
    CreateKernelTexture();
    CreateNoiseTexture();
}

void RENDER_PASS_SSAO::Render()
{
    BeginRenderPass();

    if (gSSAODebugVariables.turnOffSSAO) {
        glm::vec4 clearColor = { 1.f, 0.f, 0.f, 0.f };
        pDrvInterface->ClearBackBuffer(clearColor);
    } else {
        pDrvInterface->SetShader(EFFECT_DATA::SHR_SSAO);
        pDrvInterface->SetTexture(&m_kernelTexture, 30);
        pDrvInterface->SetTexture(&m_noiseTexture, 31);
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_COMMON_DATA);

        EFFECT_DATA::CB_CUSTOM_STRUCT ssaoData;
        ssaoData.cb0.x = 4; //kernel tex side
        ssaoData.cb0.y = gSSAODebugVariables.radius;
        ssaoData.cb0.z = gSSAODebugVariables.bias;
        pDrvInterface->FillConstBuffer(EFFECT_DATA::CB_CUSTOM, &ssaoData, sizeof(ssaoData));
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_CUSTOM);

        pDrvInterface->SetVertexFormat(EMPTY_VERTEX::formatId);

        pDrvInterface->DrawFullscreen();
    }
    EndRenderPass();
}

void RENDER_PASS_SSAO::BeginRenderPass()
{
    pRenderTargetManager->SetTextureAsRenderTarget(RT_SSAO_MASK, 0, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);
    pRenderTargetManager->SetTextureAsSRV(RT_GBUFFER_NORMAL, 21);
    pRenderTargetManager->SetTextureAsSRV(RT_DEPTH_BUFFER, 25);

    pDrvInterface->BeginRenderPass();
}

void RENDER_PASS_SSAO::EndRenderPass()
{
    pDrvInterface->EndRenderPass();
    pRenderTargetManager->ReturnAllRenderTargetsToPool();
}

void RENDER_PASS_SSAO::CreateKernelTexture()
{
    const uint32_t KERNEL_SIDE = 4;
    const uint32_t KERNEL_TEXELS = KERNEL_SIDE * KERNEL_SIDE;
    std::array<glm::i8vec4, KERNEL_TEXELS> kernelVectorsCompressed;
    uint32_t vectorId = 0;
    for (auto& compressedRandVec : kernelVectorsCompressed)
    {
        glm::vec4 randVec;
        randVec.x = GetRandomValue(-1.f, 1.f);
        randVec.y = GetRandomValue(-1.f, 1.f);
        randVec.z = GetRandomValue( 0.f, 1.f);
        randVec.w = 0.f;
        randVec = glm::normalize(randVec);

        //put some samples closer to fragment
        float scale = float(vectorId++) / KERNEL_TEXELS;
        scale = glm::mix(0.1f, 1.f, scale * scale);
        randVec *= scale;

        compressedRandVec = 127.f * randVec;
    }

    VULKAN_TEXTURE_CREATE_DATA kernelTextureCreateData;
    kernelTextureCreateData.dataSize = sizeof(glm::i8vec4) * kernelVectorsCompressed.size();
    kernelTextureCreateData.extent.height = KERNEL_SIDE;
    kernelTextureCreateData.extent.width = KERNEL_SIDE;
    kernelTextureCreateData.extent.depth = 1;
    kernelTextureCreateData.format = VK_FORMAT_R8G8B8A8_SNORM;
    kernelTextureCreateData.mipLevels = 1;
    kernelTextureCreateData.usage = VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    kernelTextureCreateData.pData = reinterpret_cast<uint8_t*>(kernelVectorsCompressed.data());
    VkResult result = pDrvInterface->CreateTexture(kernelTextureCreateData, m_kernelTexture);
    ASSERT_MSG(result == VK_SUCCESS, "Can't create SSAO kernel texure");
}

void RENDER_PASS_SSAO::CreateNoiseTexture()
{
    const uint32_t NOISE_SIDE = 4;
    const uint32_t NOISE_TEXELS = NOISE_SIDE * NOISE_SIDE;
    std::array<glm::i8vec2, NOISE_TEXELS> noiseVectorsCompressed;
    for (auto& randVecCompressed : noiseVectorsCompressed)
    {
        glm::vec2 randVec;
        randVec.x = GetRandomValue(-1.f, 1.f);
        randVec.y = GetRandomValue(-1.f, 1.f);

        randVecCompressed = 127.f * glm::normalize(randVec);
    }

    VULKAN_TEXTURE_CREATE_DATA noiseTextureCreateData;
    noiseTextureCreateData.dataSize = sizeof(glm::i8vec2) * noiseVectorsCompressed.size();
    noiseTextureCreateData.extent.height = NOISE_SIDE;
    noiseTextureCreateData.extent.width = NOISE_SIDE;
    noiseTextureCreateData.extent.depth = 1;
    noiseTextureCreateData.format = VK_FORMAT_R8G8_SNORM;
    noiseTextureCreateData.mipLevels = 1;
    noiseTextureCreateData.usage = VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    noiseTextureCreateData.pData = reinterpret_cast<uint8_t*>(noiseVectorsCompressed.data());
    VkResult result = pDrvInterface->CreateTexture(noiseTextureCreateData, m_noiseTexture);
    ASSERT_MSG(result == VK_SUCCESS, "Can't create SSAO noise texure");
}
