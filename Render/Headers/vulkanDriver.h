#pragma once

#define NOMINMAX
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

#include "effectData.h"
#include "vulkanResourcesDescription.h"

const uint32_t NUM_FRAME_BUFFERS = 2;
const uint32_t NUM_CONSTANT_BUFFERS = 16;
const uint32_t MAX_RENDER_TARGETS = 4;

struct QUEUE_FAMILIES {
    struct QUEUE_FAMILY_CREATE_PARAMS {
        std::optional<uint32_t> graphicsFamilyIndex;
        std::optional<uint32_t> presentFamilyIndex;

        bool IsComplete() {
            return graphicsFamilyIndex.has_value() && presentFamilyIndex.has_value();
        }
    };

    QUEUE_FAMILY_CREATE_PARAMS createParams;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
};

struct SWAP_CHAIN {
    struct SWAP_CHAIN_CREATE_PARAMS 
    {
        void ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        void ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        void ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
        VkExtent2D extent;
        uint32_t minImageCount, maxImageCount;
    };

    VULKAN_TEXTURE swapChainTexture[NUM_FRAME_BUFFERS];
    SWAP_CHAIN_CREATE_PARAMS createParams;
    VkSwapchainKHR swapChain;
    uint32_t curSwapChainImageId;
};

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
    seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hash_combine(seed, rest), ...);
}

struct PIPLINE_LAYOUT_STATE {
    PIPLINE_LAYOUT_STATE() : shaderId(EFFECT_DATA::SHR_LAST) {}

    uint8_t shaderId;

    size_t GetHashValue() const {
        size_t seed = 0;
        hash_combine(seed, shaderId);
        return seed;
    }
};

struct RENDER_PASS_STATE {
    RENDER_PASS_STATE()
    {
        ClearState();
    }

    void ClearState() {
        useRT.reset(); 
        useDB = false;
        for (int rtId = 0; rtId < MAX_RENDER_TARGETS; rtId++) {
            rtAttachDesc[rtId].format = VK_FORMAT_UNDEFINED;
            rtAttachDesc[rtId].loadOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
            rtAttachDesc[rtId].storeOp = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
            rtAttachDesc[rtId].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            rtAttachDesc[rtId].finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        dbAttachDesc.format = VK_FORMAT_UNDEFINED;
        dbAttachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
        dbAttachDesc.storeOp = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
        dbAttachDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        dbAttachDesc.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    size_t GetHashValue() const {
        size_t seed = 0;
        hash_combine(seed, useRT.to_ulong(), useDB);
        for (int rtId = 0; rtId < MAX_RENDER_TARGETS; rtId++) {
            if (useRT[rtId]) {
                hash_combine(seed, rtAttachDesc[rtId].format);
                hash_combine(seed, rtAttachDesc[rtId].loadOp, rtAttachDesc[rtId].storeOp);
                hash_combine(seed, rtAttachDesc[rtId].initialLayout, rtAttachDesc[rtId].finalLayout);
            }
        }
        if (useDB) {
            hash_combine(seed, dbAttachDesc.format);
            hash_combine(seed, dbAttachDesc.loadOp, dbAttachDesc.storeOp);
            hash_combine(seed, dbAttachDesc.initialLayout, dbAttachDesc.finalLayout);
        }
        return seed;
    }

    std::bitset<MAX_RENDER_TARGETS> useRT;
    bool useDB;
    std::array<VkAttachmentDescription, MAX_RENDER_TARGETS> rtAttachDesc;
    VkAttachmentDescription dbAttachDesc;
};

struct FRAME_BUFFER_STATE {
    FRAME_BUFFER_STATE() : pDepthTexture(nullptr), renderPassId(0) {
        pRenderTargetTextures.fill(nullptr);
    }

    uint32_t GetFrameBufferHeight() const;
    uint32_t GetFrameBufferWigth() const;
    glm::uvec2 GetFrameBufferSize() const;

    void ClearState()
    {
        pRenderTargetTextures.fill(nullptr);
        pDepthTexture = nullptr;
        renderPassId = -1;
    }

    size_t GetHashValue() const {
        size_t seed = 0;
        hash_combine(seed, renderPassId);
        for (int i = 0; i < MAX_RENDER_TARGETS; i++) {
            if (pRenderTargetTextures[i]) {
                hash_combine(seed, i);
                hash_combine(seed, pRenderTargetTextures[i]->image);
            }
        }
        if (pDepthTexture) {
            hash_combine(seed, pDepthTexture->image);
        }
        return seed;
    }

    std::array<const VULKAN_TEXTURE*, MAX_RENDER_TARGETS> pRenderTargetTextures;
    const VULKAN_TEXTURE* pDepthTexture;
    size_t renderPassId;
};

struct DEPTH_STATE {
    uint8_t depthTestEnable : 1;
    uint8_t depthWriteEnable : 1;
    uint8_t depthCompareOp : 1;
    uint8_t stencilTestEnable : 1;
};

struct PIPLINE_STATE {
    uint8_t vertexFormatId;
    uint8_t shaderId;
    std::bitset<8> dynamicFlagsBitset;
    uint32_t viewportWidth;
    uint32_t viewportHeight;
    union
    {
        DEPTH_STATE depthStateState;
        uint8_t     depthStateMask;
    };
    size_t  piplineLayoutId;
    size_t  renderPassId;

    size_t GetHashValue() const {
        size_t seed = 0;
        hash_combine(seed, vertexFormatId, shaderId, dynamicFlagsBitset.to_ulong(), depthStateMask, piplineLayoutId);
        hash_combine(seed, viewportWidth, viewportHeight);
        return seed;
    }
};


class VULKAN_DRIVER_INTERFACE {
public:
    bool Init();
    void Term();

    VkResult InitPipelineState();
    void     InvalidateDeviceState();

    void StartFrame();
    void EndFrame();
    
    void WaitGPU();
    void DropPiplineStateCache();
    void SubmitCommandBuffer();

    void ClearBackBuffer(const glm::vec4& clearColor);

    void SetVertexBuffer(VULKAN_BUFFER vertexBuffer, uint32_t offset);
    void SetIndexBuffer(VULKAN_BUFFER indexBuffer, uint32_t offset);

    void FillPushConstantBuffer(const void* pData, uint32_t dataSize);
    void FillConstBuffer(uint32_t bufferId, const void* pData, uint32_t dataSize);
    void SetConstBuffer(uint32_t bufferId);
    void SetTexture(const VULKAN_TEXTURE* texture, uint32_t slot);
    void SetShader(uint8_t shaderId);
    void SetVertexFormat(uint8_t vertexFormat);

    void SetRenderTarget(const VULKAN_TEXTURE* pRtTexture, uint32_t rtSlot, VkAttachmentDescription rtDesc);
    void SetDepthBuffer(const VULKAN_TEXTURE* pDbTexture, VkAttachmentDescription depthDesc);

    void SetDynamicState(VkDynamicState state, bool enableState);
    void SetDepthTestState(bool depthTestEnable);
    void SetDepthWriteState(bool depthWriteEnable);
    void SetDepthComparitionOperation(bool depthCompare);
    void SetStencilTestState(bool stencilTestEnable);
    void SetScissorRect(const VkRect2D& scissorRect);
    void SetDepthBiasParams(float depthBiasConstant, float depthBiasSlope);
    
    void BeginRenderPass();
    void EndRenderPass();
    void ChangeTextureLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VULKAN_TEXTURE& texture);

    void Draw(uint32_t vertexesNum);
    void DrawIndexed(uint32_t indexesNum, uint32_t vertexBufferOffset = 0, uint32_t indexBufferOffset = 0);
    void DrawFullscreen();

    uint32_t GetMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkDescriptorSetLayoutBinding CreateLayoutBinding(uint32_t bindingSlot, VkDescriptorType descriptorType, VkShaderStageFlagBits stageBitFlags);
    bool CreateShader(const std::vector<char>& shaderRawData, VkShaderModule& shaderModule) const;
    void DestroyShader(VkShaderModule& shaderModule) const;

    VkResult CreateRenderTarget(VULKAN_TEXTURE_CREATE_DATA& createData, VULKAN_TEXTURE& texture);
    VkResult CreateTexture(VULKAN_TEXTURE_CREATE_DATA& createData, VULKAN_TEXTURE& texture);
    void     DestroyTexture(VULKAN_TEXTURE& texture);

    uint32_t GetBackBufferWidth() const { return m_swapChain.createParams.extent.width; }
    uint32_t GetBackBufferHeight() const { return m_swapChain.createParams.extent.height; }
    VkFormat GetBackBufferFormat() const { return m_swapChain.createParams.surfaceFormat.format; }

    VkResult      CreateBuffer  (const VkBufferCreateInfo& bufferInfo, bool isUpdatedByCPU, VULKAN_BUFFER& createdBuffer);
    VkResult      CreateAndFillBuffer(const VkBufferCreateInfo& bufferInfo, const uint8_t* pSourceData, bool isUpdatedByCPU, VULKAN_BUFFER& createdBuffer);
    VkResult      FillBuffer    (const uint8_t* rawData, uint64_t rawDataSize, uint64_t offset, VULKAN_BUFFER& buffer);
    void          CopyBuffer    (VULKAN_BUFFER srcBuffer, VULKAN_BUFFER dstBuffer, VkDeviceSize size);
    void          DestroyBuffer (VULKAN_BUFFER& buffer);

    VkResult AllocateMemory (const VkMemoryAllocateInfo& allocationInfo, VkDeviceMemory& allocatedMemory);
    void     FreeMemory (VkDeviceMemory& allocatedMemory);

    float     GetFrameGpuTime()    const { return m_frameGpuTime; }
    const VULKAN_TEXTURE & GetCurSwapChainTexture () const { return m_swapChain.swapChainTexture[m_swapChain.curSwapChainImageId]; }
private:
    //functions
    std::vector<const char*> GetRequiredInstanceExtentions() const;
    std::vector<const char*> GetRequiredInstanceLayers() const;
    std::vector<const char*> GetRequiredDeviceExtentions() const;

    VkResult InitInstance();
    VkResult InitPhysicalDevice();
    VkResult InitLogicalDevice();
    VkResult InitDebugMessenger();
    VkResult InitWindowSurface();
    VkResult InitSwapChain();
    VkResult InitSwapChainImages();
    VkResult InitSwapChainFrameBuffers();
    VkResult InitCommandBuffers();
    VkResult InitSemaphoresAndFences();
    VkResult InitIntermediateBuffers();
    VkResult InitConstBuffers();
    VkResult InitSamplers();

    VkResult CreateDecriptorPools();
    VkResult CreateDecsriptorSetLayout(uint8_t shaderId);
    VkResult CreatePiplineLayout(const PIPLINE_LAYOUT_STATE& piplineLayoutKey);
    VkResult CreateGraphicPipeline(const PIPLINE_STATE& psoKey);
    VkResult CreateRenderPass(const RENDER_PASS_STATE& rtState);
    VkResult CreateFrameBuffer(const FRAME_BUFFER_STATE& frameBufferState);

    VkResult RecreateSwapChain();

    void TermIntermediateBuffers();
    void TermConstBuffers();
    void TermSamplers();
    void TermSwapChain();

    void            SetupCurrentCommandBuffer(VkCommandBuffer newCurrentBuffer);
    VkCommandBuffer BeginSingleTimeCommands();
    void            EndSingleTimeCommands(VkCommandBuffer commandBuffer);

    uint32_t GetDeviceCoherentValue(uint32_t bufferSize) const;

    VkResult CreateTextureImage(const VkImageCreateInfo& imageInfo, VULKAN_TEXTURE& createdTexure);
    VkResult CopyBufferToTexture(const VULKAN_BUFFER& buffer, VkDeviceSize bufferOffset, VULKAN_TEXTURE& texture, uint32_t mipLevel);
    VkResult CreateImageView(const VkImageViewCreateInfo& imageViewInfo, VULKAN_TEXTURE& texture);

    void     SetupSamples();
    VkResult CreateSampler(VkFilter minMagFilter, VkSamplerMipmapMode mapFilter, VkSamplerAddressMode addressMode, 
        VkCompareOp compareOp, float anisoParam, VkSampler& sampler);
    void     DestroySampler(VkSampler& sampler);

    bool     UpdatePiplineState();

    int DeviceSuitabilityRate(const VkPhysicalDevice& device) const;
    QUEUE_FAMILIES::QUEUE_FAMILY_CREATE_PARAMS FindQueueFamilies(const VkPhysicalDevice& device) const;
    SWAP_CHAIN::SWAP_CHAIN_CREATE_PARAMS GetSwapChainCreateParams(const VkPhysicalDevice& device) const;

    void TermDebugMessenger();

    //fields
private:
    float    m_frameGpuTime;
    uint64_t m_frameId;
    uint32_t m_curContextId;
    bool     m_enableValidationLayer;

    VkInstance       m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice         m_device;
	QUEUE_FAMILIES   m_queueFamilies;

    VkPhysicalDeviceProperties m_deviceProperties;
    VkPhysicalDeviceFeatures   m_deviceFeatures;

    VkSurfaceKHR                                  m_windowSurface;
    SWAP_CHAIN                                    m_swapChain;

    VkCommandBuffer m_curCommandBuffer;
    VkCommandPool   m_commandPool;
	VkCommandBuffer m_commandBuffers[NUM_FRAME_BUFFERS];

    VkSemaphore m_imageAvailableSemaphore[NUM_FRAME_BUFFERS];
    VkSemaphore m_renderFinishedSemaphore[NUM_FRAME_BUFFERS];
	VkFence     m_cpuGpuSyncFence[NUM_FRAME_BUFFERS];
    VkFence     m_waitGpuFence;

    //todo:: make one buffer for everything https://developer.nvidia.com/vulkan-memory-management
    std::array<uint32_t, NUM_CONSTANT_BUFFERS>          m_constBufferLastRecordOffset;
    std::array<uint32_t, NUM_CONSTANT_BUFFERS>          m_constBufferOffsets;
    std::array<VULKAN_BUFFER, NUM_CONSTANT_BUFFERS>  m_constBuffers;
    std::array<VkSampler, EFFECT_DATA::SAMPLER_LAST>              m_samplers;

    //used for intermediate stage of creating texture
    VULKAN_BUFFER m_intermediateStagingBuffer;

    //todo: validate all resource descriptors at the same time
    std::vector<std::pair<uint8_t, VkDescriptorImageInfo>>              m_curPassImageDescriptors;
    std::vector<std::pair<uint8_t, VkDescriptorBufferInfo>>             m_curPassBufferDescriptors;
    std::array<std::pair<uint8_t, VkDescriptorImageInfo>, EFFECT_DATA::SAMPLER_LAST> m_samplerDescriptors;

    uint32_t                                       m_pushConstantBufferDirtySize;
    std::array<uint8_t, 128>                       m_pushConstantBuffer;
    VkRect2D                                       m_dynamicScissorRect;
    glm::vec2                                      m_dynamicBiasParams;

    bool                                           m_isDynamicDepthBiasDirty;
    bool                                           m_isDynamicScissorRectDirty;

    std::array<VkDescriptorPool, NUM_FRAME_BUFFERS>                 m_descriptorPool;
    std::array<VkDescriptorSetLayout, EFFECT_DATA::SHR_LAST>        m_descriptorSetLayout;
    std::unordered_map<size_t, VkRenderPass>     m_renderPassCache;
    std::unordered_map<size_t, VkFramebuffer>    m_frameBufferCache;
    std::unordered_map<size_t, VkPipelineLayout> m_pipelineLayoutCache;
    std::unordered_map<size_t, VkPipeline>       m_pipelineStateCache;
    
    RENDER_PASS_STATE     m_curRenderPassState;
    VkRenderPass          m_curRenderPass;
    FRAME_BUFFER_STATE    m_curFrameBufferState;
    VkFramebuffer         m_curFrameBuffer;
    VkDescriptorSet       m_curDescriptorSet;
    PIPLINE_LAYOUT_STATE  m_curPiplineLayoutState;
    VkPipelineLayout      m_curPiplineLayout;
    PIPLINE_STATE         m_curPiplineState;
    VkPipeline            m_curPipeline;

    bool                  m_updatePiplineLayout;
    bool                  m_updatePiplineState;
    bool                  m_updateDescriptorSet;
    
    VULKAN_BUFFER                m_curVertexBuffer;
    VULKAN_BUFFER                m_curIndexBuffer;

    size_t                   m_defaultRenderPassHashValue;
    VULKAN_TEXTURE           m_emptyTexture;

    VkDebugUtilsMessengerEXT m_debugMessenger;

};

extern std::unique_ptr<VULKAN_DRIVER_INTERFACE> pDrvInterface;
extern std::function<void(uint32_t& windowWidth, uint32_t& windowHeight, void*& pWindow)> pCallbackGetWindowParams;
