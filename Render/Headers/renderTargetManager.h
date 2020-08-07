#pragma once
#include "renderTargetEnum.h"
#include "vulkanDriver.h"

//todo: plz, rewrite get/return mechanism for render targets!
class RENDER_TARGET_MANAGER {
public:
    bool Init(uint32_t backBufferWidth, uint32_t backBufferHeight);
    void Resize(uint32_t backBufferWidth, uint32_t backBufferHeight);
    void Term();

    void StartFrame();
    void SetBackBufferTexture(const VULKAN_TEXTURE& backBufferTexture) { m_renderTargetsState[RT_BACK_BUFFER].texture = backBufferTexture; }

    uint32_t GetRenderTargetWidht(RENDER_TARGET rtIndex) const { return rtIndex == RT_BACK_BUFFER ? pDrvInterface->GetBackBufferWidth() : m_renderTargetsState[rtIndex].texture.width; }
    uint32_t GetRenderTargetHeight(RENDER_TARGET rtIndex) const { return rtIndex == RT_BACK_BUFFER ? pDrvInterface->GetBackBufferHeight() : m_renderTargetsState[rtIndex].texture.height; }
    VkFormat GetRenderTargetFormat(RENDER_TARGET rtIndex) const { return  rtIndex == RT_BACK_BUFFER ? pDrvInterface->GetBackBufferFormat() : m_renderTargetsState[rtIndex].texture.format; }
    VkImageLayout GetRenderTargetPrevLayout(RENDER_TARGET rtIndex) { return m_renderTargetsState[rtIndex].layout; }
    VkImageLayout GetRenderTargetFutureLayout(RENDER_TARGET rtIndex) { return m_renderTargetsState[rtIndex].futureLayout; }

    const VULKAN_TEXTURE& GetRenderTarget(RENDER_TARGET rtIndex, VkAccessFlags accessFlags, VkImageLayout layout);
    void ReturnRenderTarget(RENDER_TARGET rtIndex);
    void ReturnAllRenderTargets();
private:
    struct RENDER_TARGET_STATE {
        void InvatidateState() {
            layout = futureLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            accessFlags = futureAccessFlags = VK_ACCESS_FLAG_BITS_MAX_ENUM;
        }

        VkImageLayout  layout;
        VkImageLayout  futureLayout;

        VkAccessFlags  accessFlags;
        VkAccessFlags  futureAccessFlags;

        VULKAN_TEXTURE texture;
    };

    //VkAccessFlags
    static const int MAX_RENDER_TARGETS_NUM = 4;
    std::bitset<MAX_RENDER_TARGETS_NUM> m_renderTargetsAvailabilityMask;
    std::array<RENDER_TARGET_STATE, MAX_RENDER_TARGETS_NUM> m_renderTargetsState;
    
    //back buffer params
    uint32_t m_backBufferWight;
    uint32_t m_backBufferHeight;
};

extern std::unique_ptr< RENDER_TARGET_MANAGER> pRenderTargetManager;
