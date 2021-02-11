#pragma once
#include "renderTargetEnum.h"
#include "vulkanDriver.h"

//todo: plz, rewrite get/return mechanism for render targets!
class RENDER_TARGET_MANAGER {
public:
    bool Init(uint32_t backBufferWidth, uint32_t backBufferHeight, VkFormat backBufferFormat);
    void Resize(uint32_t backBufferWidth, uint32_t backBufferHeight, VkFormat backBufferFormat);
    void Term();

    void StartFrame();
    void EndFrame();

    void SetTextureAsRenderTarget(RENDER_TARGET_ID rtIndex, size_t slotIndex, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp);
    void SetTextureAsDepthBuffer(RENDER_TARGET_ID rtIndex, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp);
    void SetTextureAsSRV(RENDER_TARGET_ID rtIndex, size_t slotIndex);

    void ReturnRenderTarget(RENDER_TARGET_ID rtIndex);
    void ReturnAllRenderTargetsToPool();
private:
    void ObtainRenderTarget(RENDER_TARGET_ID rtIndex, VkAccessFlags accessFlags, VkImageLayout layout);
private:
    std::bitset<RT_LAST>                         m_renderTargetsAvailabilityMask;
    std::array<VkAttachmentDescription, RT_LAST> m_renderTargetDesc;
    std::array<VULKAN_TEXTURE,          RT_LAST> m_renderTargetList;
};

extern std::unique_ptr<RENDER_TARGET_MANAGER> pRenderTargetManager;
