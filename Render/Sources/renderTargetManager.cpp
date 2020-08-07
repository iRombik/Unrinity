#include "renderTargetManager.h"

std::unique_ptr< RENDER_TARGET_MANAGER> pRenderTargetManager;

bool RENDER_TARGET_MANAGER::Init(uint32_t backBufferWidth, uint32_t backBufferHeight)
{
    m_renderTargetsAvailabilityMask.set();
    for (auto& rtState : m_renderTargetsState) {
        rtState.InvatidateState();
    }

    bool isInited = true;
    {
        VULKAN_TEXTURE_CREATE_DATA depthBufferCreateData(VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, backBufferWidth, backBufferHeight);
        VkResult result = pDrvInterface->CreateRenderTarget(depthBufferCreateData, m_renderTargetsState[RT_DEPTH_BUFFER].texture);
        ASSERT(result == VK_SUCCESS);
        isInited &= result == VK_SUCCESS;
    }

    {
        VULKAN_TEXTURE_CREATE_DATA fp16CreateData(VK_FORMAT_R16G16B16A16_SFLOAT, VkImageUsageFlagBits(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT), backBufferWidth, backBufferHeight);
        VkResult result = pDrvInterface->CreateRenderTarget(fp16CreateData, m_renderTargetsState[RT_FP16].texture);
        ASSERT(result == VK_SUCCESS);
        isInited &= result == VK_SUCCESS;
    }

    {
        const uint32_t SHADOW_MAP_SIZE = 2048;
        VULKAN_TEXTURE_CREATE_DATA shadowMapCreateData(VK_FORMAT_D16_UNORM, VkImageUsageFlagBits(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT),
            SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
        VkResult result = pDrvInterface->CreateRenderTarget(shadowMapCreateData, m_renderTargetsState[RT_DEPTH_BUFFER].texture);
        ASSERT(result == VK_SUCCESS);
        isInited &= result == VK_SUCCESS;
    }

    if (isInited) {
        m_backBufferHeight = backBufferHeight;
        m_backBufferWight = backBufferWidth;
    }

    return isInited;
}

void RENDER_TARGET_MANAGER::Resize(uint32_t backBufferWidth, uint32_t backBufferHeight)
{
    if (backBufferHeight == m_backBufferHeight && backBufferWidth == m_backBufferWight) {
        return;
    }
}

void RENDER_TARGET_MANAGER::Term()
{
    static_assert(RT_BACK_BUFFER + 1 == RT_LAST);
    for (int i = 0; i < RT_BACK_BUFFER; i++) {
        pDrvInterface->DestroyTexture(m_renderTargetsState[i].texture);
    }
}

void RENDER_TARGET_MANAGER::StartFrame()
{
    ASSERT(m_renderTargetsAvailabilityMask.all());
    for (auto& rtState : m_renderTargetsState) {
        rtState.InvatidateState();
    }
}

const VULKAN_TEXTURE& RENDER_TARGET_MANAGER::GetRenderTarget(RENDER_TARGET rtIndex, VkAccessFlags accessFlags, VkImageLayout layout)
{
    ASSERT(m_renderTargetsAvailabilityMask.test(rtIndex));
    m_renderTargetsAvailabilityMask.reset(rtIndex);
    RENDER_TARGET_STATE& rtState = m_renderTargetsState[rtIndex];
    rtState.futureLayout = layout;
    rtState.accessFlags = accessFlags;
    if (rtState.layout != rtState.futureLayout) {
        pDrvInterface->ChangeTextureLayout(rtState.layout, rtState.futureLayout, rtState.texture);
    }
    return rtState.texture;
}

void RENDER_TARGET_MANAGER::ReturnRenderTarget(RENDER_TARGET rtIndex)
{
    RENDER_TARGET_STATE& rtState = m_renderTargetsState[rtIndex];
    rtState.layout = rtState.futureLayout;
    rtState.accessFlags = rtState.futureAccessFlags;
    m_renderTargetsAvailabilityMask.set(rtIndex);
}

void RENDER_TARGET_MANAGER::ReturnAllRenderTargets()
{
    for (int rtId = 0; rtId < RT_LAST; rtId++) {
        if (!m_renderTargetsAvailabilityMask.test(rtId)) {
            ReturnRenderTarget((RENDER_TARGET)rtId);
        }
    }
}
