#pragma once

#define NOMINMAX
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

#include "effectData.h"
#include "renderTargetEnum.h"

const uint32_t NUM_FRAME_BUFFERS = 2;
const uint32_t NUM_CONSTANT_BUFFERS = 3;
const uint32_t NUM_SAMPLERS = 2;

struct VULKAN_BUFFER
{
    VULKAN_BUFFER() : bufferSize(0), realBufferSize(0), buffer(), bufferMemory() {}
    bool operator==(const VULKAN_BUFFER& vkBuf) const {
        return buffer == vkBuf.buffer;
    }
    bool operator!=(const VULKAN_BUFFER& vkBuf) const {
        return !operator==(vkBuf);
    }

    size_t   bufferSize;
    size_t   realBufferSize;
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
};

struct VULKAN_TEXTURE_CREATE_DATA
{
    VULKAN_TEXTURE_CREATE_DATA() : extent{ 0, 0, 0}, mipLevels(0), format(VK_FORMAT_UNDEFINED), usage(VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM), pData(nullptr), dataSize(0) {}
    VULKAN_TEXTURE_CREATE_DATA(VkFormat _format, VkImageUsageFlagBits _usage, uint32_t _width, uint32_t _height, uint32_t _depth = 1) 
        : extent{ _width, _height, _depth }, mipLevels(1), format(_format), usage(_usage), pData(nullptr), dataSize(0) {}

    uint8_t* pData;
    size_t dataSize;
    VkExtent3D extent;
    VkFormat format;
    uint32_t mipLevels;
    std::vector<size_t> mipLevelsOffsets;
    VkImageUsageFlagBits usage;
};

struct VULKAN_TEXTURE
{
    VULKAN_TEXTURE () : width(0), height(0), format(VK_FORMAT_UNDEFINED), mipLevels(0), image(VK_NULL_HANDLE), imageMemory(VK_NULL_HANDLE), imageView(VK_NULL_HANDLE) {}

    uint32_t width;
    uint32_t height;
    VkFormat format;
    uint8_t mipLevels;

    VkImage image;
    VkImageView imageView;
    VkDeviceMemory imageMemory;
};

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
    struct SWAP_CHAIN_CREATE_PARAMS {
        void ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        void ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        void ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
        VkExtent2D extent;
        uint32_t minImageCount, maxImageCount;
    };

    SWAP_CHAIN_CREATE_PARAMS createParams;
    VkSwapchainKHR swapChain;
    std::array<VULKAN_TEXTURE, NUM_FRAME_BUFFERS> swapChainTexture;
    std::array<size_t, NUM_FRAME_BUFFERS>         swapChainFramebufferHashValue;
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
    RENDER_PASS_STATE() : useRT(false), useDB(false)
    {
        rtAttachDesc.format = VK_FORMAT_UNDEFINED;
        rtAttachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
        rtAttachDesc.storeOp = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
        rtAttachDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        rtAttachDesc.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        dbAttachDesc.format = VK_FORMAT_UNDEFINED;
        dbAttachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
        dbAttachDesc.storeOp = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
        dbAttachDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        dbAttachDesc.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    bool useRT;
    bool useDB;
    VkAttachmentDescription rtAttachDesc;
    VkAttachmentDescription dbAttachDesc;

    size_t GetHashValue() const {
        size_t seed = 0;
        hash_combine(seed, useRT, useDB);
        if (useRT) {
            hash_combine(seed, rtAttachDesc.format, rtAttachDesc.loadOp, rtAttachDesc.storeOp, rtAttachDesc.initialLayout, rtAttachDesc.finalLayout);
        }
        if (useDB) {
            hash_combine(seed, dbAttachDesc.format, dbAttachDesc.loadOp, dbAttachDesc.storeOp, dbAttachDesc.initialLayout, dbAttachDesc.finalLayout);
        }
        return seed;
    }
};

struct FRAME_BUFFER_STATE {
    FRAME_BUFFER_STATE() : rtIndex(RT_LAST), dbIndex(RT_LAST), rtView(VK_NULL_HANDLE), dbView(VK_NULL_HANDLE), renderPassId(0) {}

    RENDER_TARGET rtIndex;
    RENDER_TARGET dbIndex;
    VkImageView rtView;
    VkImageView dbView;
    size_t renderPassId;

    size_t GetHashValue() const {
        size_t seed = 0;
        hash_combine(seed, rtView, dbView, renderPassId);
        return seed;
    }
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
    uint8_t dynamicFlagsMask;
    union
    {
        DEPTH_STATE depthStateS;
        uint8_t     depthStateU;
    };
    size_t  piplineLayoutId;
    size_t  renderPassId;

    size_t GetHashValue() const {
        size_t seed = 0;
        hash_combine(seed, vertexFormatId, shaderId, dynamicFlagsMask, depthStateU, piplineLayoutId);
        return seed;
    }
};


class VULKAN_DRIVER_INTERFACE {
public:
    bool Init();
    void Term();

    VkResult InitPipelineState();
    void     InvalidateDeviceState();

    void StartFrame(double frameStartTime);
	void EndFrame(double frameEndTime);
	void SubmitCommandBuffer();

    void SetVertexBuffer(VULKAN_BUFFER vertexBuffer, uint32_t offset);
    void SetIndexBuffer(VULKAN_BUFFER indexBuffer, uint32_t offset);

    void FillPushConstantBuffer(const void* pData, uint32_t dataSize);
    void FillConstBuffer(uint32_t bufferId, const void* pData, uint32_t dataSize);
    void SetConstBuffer(uint32_t bufferId);
    void SetTexture(const VULKAN_TEXTURE* texture, uint32_t slot);
    void SetShader(uint8_t shaderId);
    void SetVertexFormat(uint8_t vertexFormat);

    void SetRenderTarget(RENDER_TARGET rtIndex, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp);
    void RemoveRenderTarget();
    void SetRenderTargetAsShaderResource(RENDER_TARGET rtIndex, uint32_t slot);

    void SetDepthBuffer(RENDER_TARGET rtIndex, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp);
    void RemoveDepthBuffer();

    void SetDynamicState(VkDynamicState state, bool enableState);
    void SetDepthTestState(bool depthTestEnable);
    void SetDepthWriteState(bool depthWriteEnable);
    void SetDepthComparitionOperation(bool depthCompare);
    void SetStencilTestState(bool stencilTestEnable);
    void SetScissorRect(const VkRect2D& scissorRect);
    
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

//     VkFramebuffer& GetBackBufferFrameBuffer();
    uint32_t GetBackBufferWidth() const { return m_swapChain.createParams.extent.width; }
    uint32_t GetBackBufferHeight() const { return m_swapChain.createParams.extent.height; }
    VkFormat GetBackBufferFormat() const { return m_swapChain.swapChainTexture[0].format; }

    VkResult      CreateBuffer  (const VkBufferCreateInfo& bufferInfo, bool isUpdatedByCPU, VULKAN_BUFFER& createdBuffer);
    VkResult      CreateAndFillBuffer(const VkBufferCreateInfo& bufferInfo, const uint8_t* pSourceData, bool isUpdatedByCPU, VULKAN_BUFFER& createdBuffer);
    VkResult      FillBuffer    (const uint8_t* rawData, uint64_t rawDataSize, uint64_t offset, VULKAN_BUFFER& buffer);
    void          CopyBuffer    (VULKAN_BUFFER srcBuffer, VULKAN_BUFFER dstBuffer, VkDeviceSize size);
    void          DestroyBuffer (VULKAN_BUFFER& buffer);

    VkResult AllocateMemory (const VkMemoryAllocateInfo& allocationInfo, VkDeviceMemory& allocatedMemory);
    void     FreeMemory (VkDeviceMemory& allocatedMemory);

    float     GetCurTime()    const { return float(m_frameStartTime); }
private:
    //functions
    std::vector<const char*> GetRequiredInstanceExtentions() const;
    std::vector<const char*> GetRequiredInstanceLayers() const;
    std::vector<const char*> GetRequiredDeviceExtentions() const;

    VkResult InitInstance();
    VkResult InitPhysicalDevice();
    VkResult InitLogicalDevice();
    VkResult InitDebugMessenger();
    VkResult InitDefaultRenderPass();
    VkResult InitSurface();
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

    void TermIntermediateBuffers();
    void TermConstBuffers();
    void TermSamplers();

    void            SetupCurrentCommandBuffer(VkCommandBuffer newCurrentBuffer);
    VkCommandBuffer BeginSingleTimeCommands();
    void            EndSingleTimeCommands(VkCommandBuffer commandBuffer);

    size_t GetDeviceCoherentSize(size_t bufferSize) const;

    VkResult CreateTextureImage(const VkImageCreateInfo& imageInfo, VULKAN_TEXTURE& createdTexure);
    VkResult CopyBufferToTexture(const VULKAN_BUFFER& buffer, VkDeviceSize bufferOffset, VULKAN_TEXTURE& texture, uint32_t mipLevel);
    VkResult CreateImageView(const VkImageViewCreateInfo& imageViewInfo, VULKAN_TEXTURE& texture);

    void     SetupSamples();
    VkResult CreateSampler(VkFilter minMagFilter, VkSamplerMipmapMode mapFilter, VkSamplerAddressMode addressMode, float anisoParam, VkSampler& sampler);
    void     DestroySampler(VkSampler& sampler);

    bool     UpdatePiplineState();

    int DeviceSuitabilityRate(const VkPhysicalDevice& device) const;
    QUEUE_FAMILIES::QUEUE_FAMILY_CREATE_PARAMS FindQueueFamilies(const VkPhysicalDevice& device) const;
    SWAP_CHAIN::SWAP_CHAIN_CREATE_PARAMS GetSwapChainCreateParams(const VkPhysicalDevice& device) const;

    void TermDebugMessenger();

    //fields
private:
    double   m_frameStartTime;
    double   m_frameTime;
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

    //todo:: make one buffer for everything https://developer.nvidia.com/vulkan-memory-management
    std::array<VULKAN_BUFFER, NUM_CONSTANT_BUFFERS>  m_constBuffers;
    std::array<VkSampler, NUM_SAMPLERS>              m_samplers;

    //used for intermediate stage of creating texture
    VULKAN_BUFFER m_intermediateStagingBuffer;

    //todo: validate all resource descriptors at the same time
    std::vector<std::pair<uint8_t, VkDescriptorImageInfo>>              m_curPassImageDescriptors;
    std::vector<std::pair<uint8_t, VkDescriptorBufferInfo>>             m_curPassBufferDescriptors;
    std::array<std::pair<uint8_t, VkDescriptorImageInfo>, NUM_SAMPLERS> m_samplerDescriptors;

    uint32_t                                       m_pushConstantBufferDirtySize;
    std::array<uint8_t, 128>                       m_pushConstantBuffer;
    bool                                           m_isDynamicScissorRectDirty;
    VkRect2D                                       m_dynamicScissorRect;

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
    
    std::array<VkImageView, 128> m_curTextureState;
    VULKAN_BUFFER                m_curVertexBuffer;
    VULKAN_BUFFER                m_curIndexBuffer;

    size_t                   m_defaultRenderPassHashValue;
    VULKAN_TEXTURE           m_emptyTexture;

    VkDebugUtilsMessengerEXT m_debugMessenger;

};

extern std::unique_ptr<VULKAN_DRIVER_INTERFACE> pDrvInterface;
extern std::function<void(uint32_t& windowWidth, uint32_t& windowHeight, void*& pWindow)> pCallbackGetWindowParams;
