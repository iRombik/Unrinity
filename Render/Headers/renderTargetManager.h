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
    void SetCurSwapChainBufferId(uint8_t id) {
        m_curSwapChainTexture = id; m_renderTargetsState[RT_BACK_BUFFER].texture = m_swapChainTextures[id];
    }
    void SetupSwapChainTexture(const VULKAN_TEXTURE& tex, uint8_t texId) { m_swapChainTextures[texId] = tex; }

    VkImageLayout GetRenderTargetPrevLayout(RENDER_TARGET rtIndex) { return m_renderTargetsState[rtIndex].layout; }
    VkImageLayout GetRenderTargetFutureLayout(RENDER_TARGET rtIndex) { return m_renderTargetsState[rtIndex].futureLayout; }

    const VULKAN_TEXTURE* GetRenderTarget(RENDER_TARGET rtIndex, VkAccessFlags accessFlags, VkImageLayout layout);
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
    std::bitset<RT_LAST> m_renderTargetsAvailabilityMask;
    std::array<RENDER_TARGET_STATE, RT_LAST> m_renderTargetsState;
    
    //back buffer params
    std::array <VULKAN_TEXTURE, NUM_FRAME_BUFFERS> m_swapChainTextures;
    uint8_t m_curSwapChainTexture;
};

extern std::unique_ptr<RENDER_TARGET_MANAGER> pRenderTargetManager;
