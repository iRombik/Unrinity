#include "renderTargetManager.h"

std::unique_ptr< RENDER_TARGET_MANAGER> pRenderTargetManager;

bool RENDER_TARGET_MANAGER::Init(uint32_t backBufferWidth, uint32_t backBufferHeight, VkFormat backBufferFormat)
{
    m_renderTargetsAvailabilityMask.set();

    bool isInited = true;
    {
        VULKAN_TEXTURE_CREATE_DATA depthBufferCreateData(VK_FORMAT_D24_UNORM_S8_UINT,
            VkImageUsageFlagBits(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT), backBufferWidth, backBufferHeight);
        VkResult result = pDrvInterface->CreateRenderTarget(depthBufferCreateData, m_renderTargetList[RT_DEPTH_BUFFER]);
        ASSERT(result == VK_SUCCESS);
        isInited &= result == VK_SUCCESS;
    }

    {
        VULKAN_TEXTURE_CREATE_DATA fp16CreateData(VK_FORMAT_R16G16B16A16_SFLOAT, VkImageUsageFlagBits(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT), backBufferWidth, backBufferHeight);
        VkResult result = pDrvInterface->CreateRenderTarget(fp16CreateData, m_renderTargetList[RT_FP16]);
        ASSERT(result == VK_SUCCESS);
        isInited &= result == VK_SUCCESS;
    }

    {
        VULKAN_TEXTURE_CREATE_DATA gBufferAlbedoCreateData(VK_FORMAT_R8G8B8A8_UNORM, VkImageUsageFlagBits(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT), backBufferWidth, backBufferHeight);
        VkResult result = pDrvInterface->CreateRenderTarget(gBufferAlbedoCreateData, m_renderTargetList[RT_GBUFFER_ALBEDO]);
        ASSERT(result == VK_SUCCESS);
        isInited &= result == VK_SUCCESS;
    }

    {
        VULKAN_TEXTURE_CREATE_DATA gBufferNormalCreateData(VK_FORMAT_R16G16B16A16_SFLOAT, VkImageUsageFlagBits(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT), backBufferWidth, backBufferHeight);
        VkResult result = pDrvInterface->CreateRenderTarget(gBufferNormalCreateData, m_renderTargetList[RT_GBUFFER_NORMAL]);
        ASSERT(result == VK_SUCCESS);
        isInited &= result == VK_SUCCESS;
    }

    {
        VULKAN_TEXTURE_CREATE_DATA gBufferMetalnessRoughnessCreateData(VK_FORMAT_R8G8_UNORM, VkImageUsageFlagBits(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT), backBufferWidth, backBufferHeight);
        VkResult result = pDrvInterface->CreateRenderTarget(gBufferMetalnessRoughnessCreateData, m_renderTargetList[RT_GBUFFER_METALL_ROUGHNESS]);
        ASSERT(result == VK_SUCCESS);
        isInited &= result == VK_SUCCESS;
    }

    {
        VULKAN_TEXTURE_CREATE_DATA gBufferWorldPosCreateData(VK_FORMAT_R16G16B16A16_SFLOAT, VkImageUsageFlagBits(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT), backBufferWidth, backBufferHeight);
        VkResult result = pDrvInterface->CreateRenderTarget(gBufferWorldPosCreateData, m_renderTargetList[RT_GBUFFER_WORLD_POS]);
        ASSERT(result == VK_SUCCESS);
        isInited &= result == VK_SUCCESS;
    }

    {
        const uint32_t SHADOW_MAP_SIZE = 2048;
        VULKAN_TEXTURE_CREATE_DATA shadowMapCreateData(VK_FORMAT_D16_UNORM, VkImageUsageFlagBits(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT),
            SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
        VkResult result = pDrvInterface->CreateRenderTarget(shadowMapCreateData, m_renderTargetList[RT_SHADOW_MAP]);
        ASSERT(result == VK_SUCCESS);
        isInited &= result == VK_SUCCESS;
    }

    m_renderTargetList[RT_BACK_BUFFER].format = backBufferFormat;

    for (int i = 0; i < RT_LAST; i++)
    {
        auto& desc = m_renderTargetDesc[i];
        desc.flags = 0;
        desc.samples = VK_SAMPLE_COUNT_1_BIT;
        desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        desc.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.format = m_renderTargetList[i].format;
    }

    return isInited;
}

void RENDER_TARGET_MANAGER::Resize(uint32_t backBufferWidth, uint32_t backBufferHeight, VkFormat backBufferFormat)
{
    Term();
    Init(backBufferWidth, backBufferHeight, backBufferFormat);
}

void RENDER_TARGET_MANAGER::Term()
{
    static_assert(RT_BACK_BUFFER + 1 == RT_LAST);
    for (int i = 0; i < RT_BACK_BUFFER; i++) {
        pDrvInterface->DestroyTexture(m_renderTargetList[i]);
    }
}

void RENDER_TARGET_MANAGER::StartFrame()
{
    ASSERT(m_renderTargetsAvailabilityMask.all());
    m_renderTargetList[RT_BACK_BUFFER] = pDrvInterface->GetCurSwapChainTexture();
    for (auto& desc : m_renderTargetDesc) {
        desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        desc.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
}

void RENDER_TARGET_MANAGER::EndFrame()
{
    //todo
    ObtainRenderTarget(RT_BACK_BUFFER, 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    ReturnRenderTarget(RT_BACK_BUFFER);
}

void RENDER_TARGET_MANAGER::SetTextureAsRenderTarget(RENDER_TARGET_ID rtIndex, size_t slotIndex, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp)
{
    ObtainRenderTarget(
        rtIndex,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    m_renderTargetDesc[rtIndex].loadOp = loadOp;
    m_renderTargetDesc[rtIndex].storeOp = storeOp;
    pDrvInterface->SetRenderTarget(&m_renderTargetList[rtIndex], slotIndex, m_renderTargetDesc[rtIndex]);
}

void RENDER_TARGET_MANAGER::SetTextureAsDepthBuffer(RENDER_TARGET_ID rtIndex, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp)
{
    ObtainRenderTarget(
        rtIndex,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );
    m_renderTargetDesc[rtIndex].loadOp = loadOp;
    m_renderTargetDesc[rtIndex].storeOp = storeOp;
    pDrvInterface->SetDepthBuffer(&m_renderTargetList[rtIndex], m_renderTargetDesc[rtIndex]);
}

void RENDER_TARGET_MANAGER::SetTextureAsSRV(RENDER_TARGET_ID rtIndex, size_t slotIndex)
{
    ObtainRenderTarget(rtIndex, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    pDrvInterface->SetTexture(&m_renderTargetList[rtIndex], slotIndex);
}

void RENDER_TARGET_MANAGER::ObtainRenderTarget(RENDER_TARGET_ID rtIndex, VkAccessFlags accessFlags, VkImageLayout layout)
{
    ASSERT(m_renderTargetsAvailabilityMask.test(rtIndex));
    m_renderTargetsAvailabilityMask.reset(rtIndex);
    
    VkAttachmentDescription& desc = m_renderTargetDesc[rtIndex];
    desc.finalLayout = layout;
    if (desc.initialLayout != desc.finalLayout) {
        pDrvInterface->ChangeTextureLayout(desc.initialLayout, desc.finalLayout, m_renderTargetList[rtIndex]);
    }
}

void RENDER_TARGET_MANAGER::ReturnRenderTarget(RENDER_TARGET_ID rtIndex)
{
    m_renderTargetsAvailabilityMask.set(rtIndex);

    VkAttachmentDescription& desc = m_renderTargetDesc[rtIndex];
    desc.initialLayout = desc.finalLayout;
}

void RENDER_TARGET_MANAGER::ReturnAllRenderTargetsToPool()
{
    for (int rtId = 0; rtId < RT_LAST; rtId++) {
        if (!m_renderTargetsAvailabilityMask.test(rtId)) {
            ReturnRenderTarget((RENDER_TARGET_ID)rtId);
        }
    }
}
