#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkanDriver.h"
#include "support.h"
#include "resourceSystem.h"
#include "geometry.h"
#include "effectData.h"
//TODO: remove form here. create pso in resource system 
#include "shaderManager.h"
#include <renderTargetManager.h>

#include <algorithm>
#include <set>
#include <unordered_set>

#define SAFE_FUNC_WRAPPER(func, msg) if(func() != VK_SUCCESS) {ERROR_MSG(msg); return false;}

//todo: https://devblogs.nvidia.com/vulkan-dos-donts/

std::unique_ptr<VULKAN_DRIVER_INTERFACE> pDrvInterface;
std::function<void(uint32_t&, uint32_t&, void*&)> pCallbackGetWindowParams;

static const float DEPTH_BIAS_CLAMP = 1.f;

bool VULKAN_DRIVER_INTERFACE::Init()
{
	m_frameId = 0;
	m_curContextId = 0;
#ifdef OUT_DEBUG_INFO
    m_enableValidationLayer = true;
#else
    m_enableValidationLayer = false;
#endif
    SAFE_FUNC_WRAPPER(InitInstance, "Init instance error!");
    if (m_enableValidationLayer) {
        SAFE_FUNC_WRAPPER(InitDebugMessenger, "Init debug messager error!");
    }
    SAFE_FUNC_WRAPPER(InitSurface, "Init surface error!");
    SAFE_FUNC_WRAPPER(InitPhysicalDevice, "Physical device select error! Cant find sutable GPU.");
    SAFE_FUNC_WRAPPER(InitLogicalDevice, "Logical device init error!");
    SAFE_FUNC_WRAPPER(InitSwapChain, "Initialization of swap chain failed!");
    SAFE_FUNC_WRAPPER(InitSwapChainImages, "Initialization of swap chain images failed!");
    //SAFE_FUNC_WRAPPER(InitDefaultRenderPass, "Default render pass creation failed!");
    //SAFE_FUNC_WRAPPER(InitSwapChainFrameBuffers, "Initialization of swap chain frame buffers failed!");
    SAFE_FUNC_WRAPPER(InitCommandBuffers, "Init command pool failed!");
    SAFE_FUNC_WRAPPER(InitSemaphoresAndFences, "Init semapores failed!");
    SAFE_FUNC_WRAPPER(InitConstBuffers, "Const buffers allocation failed!");
    SAFE_FUNC_WRAPPER(InitIntermediateBuffers, "Intermediate buffers allocation failed!");
    SAFE_FUNC_WRAPPER(InitSamplers, "Can't create samplers!");
    return true;
}

void VULKAN_DRIVER_INTERFACE::Term()
{
    TermConstBuffers();
    TermSamplers();
    TermIntermediateBuffers();

    for (int i = 0; i < m_descriptorSetLayout.size(); i++) {
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout[i], nullptr);
    }
    for (int i = 0; i < m_descriptorPool.size(); i++) {
        vkDestroyDescriptorPool(m_device, m_descriptorPool[i], nullptr);
    }

	for (int i = 0; i < NUM_FRAME_BUFFERS; i++) {
		vkDestroySemaphore(m_device, m_renderFinishedSemaphore[i], nullptr);
		vkDestroySemaphore(m_device, m_imageAvailableSemaphore[i], nullptr);
	}
	for (int i = 0; i < NUM_FRAME_BUFFERS; i++) {
		vkDestroyFence(m_device, m_cpuGpuSyncFence[i], nullptr);
	}
    for (auto frameBuffer: m_frameBufferCache) {
        vkDestroyFramebuffer(m_device, frameBuffer.second, nullptr);
    }
    for (auto renderPass : m_renderPassCache) {
        vkDestroyRenderPass(m_device, renderPass.second, nullptr);
    }

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    for (auto& pl : m_pipelineLayoutCache) {
        vkDestroyPipelineLayout(m_device, pl.second, nullptr);
    }
    for (auto& pso : m_pipelineStateCache) {
        vkDestroyPipeline(m_device, pso.second, nullptr);
    }
    vkDestroySwapchainKHR(m_device, m_swapChain.swapChain, nullptr);
    vkDestroyDevice(m_device, nullptr);
    if (m_enableValidationLayer) {
        TermDebugMessenger();
    }
    vkDestroySurfaceKHR(m_instance, m_windowSurface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

bool VULKAN_DRIVER_INTERFACE::CreateShader(const std::vector<char>& shaderRawData, VkShaderModule& shaderModule) const
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderRawData.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderRawData.data());

    VkResult result = vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule);
    return result == VK_SUCCESS;
}

void VULKAN_DRIVER_INTERFACE::DestroyShader(VkShaderModule& shaderModule) const
{
    vkDestroyShaderModule(m_device, shaderModule, nullptr);
}


uint32_t VULKAN_DRIVER_INTERFACE::GetMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    uint32_t memoryType = -1;

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (IsEachMaskState(typeFilter, (1 << i)) &&
            IsEachMaskState(memProperties.memoryTypes[i].propertyFlags, properties)) {
            memoryType = i;
            break;
        }
    }

    ASSERT_MSG(memoryType != -1, "Not found correct memory type for allocation");
    return memoryType;
}


VkResult VULKAN_DRIVER_INTERFACE::CreateBuffer (const VkBufferCreateInfo& bufferInfo, bool isUpdatedByCPU, VULKAN_BUFFER& createdBuffer)
{
    if (!isUpdatedByCPU) {
        //buffer must be somehow filled
        ASSERT(IsEachMaskState(bufferInfo.usage, VK_BUFFER_USAGE_TRANSFER_DST_BIT));
    }

    VkResult result;
    result = vkCreateBuffer(m_device, &bufferInfo, nullptr, &createdBuffer.buffer);
    if (result != VK_SUCCESS) {
        ERROR_MSG("Failed buffer creation");
        return result;
    }

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(m_device, createdBuffer.buffer, &memReq);

    //find correct memory type
    const uint32_t cpuCoherentProp = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT; // | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    const uint32_t gpuOnlyProp = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    const uint32_t memoryPropertyMask = isUpdatedByCPU ? cpuCoherentProp : gpuOnlyProp;
    uint32_t memoryType = GetMemoryType(memReq.memoryTypeBits, memoryPropertyMask);

    if (memoryType == -1) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memoryType;

    result = AllocateMemory(allocInfo, createdBuffer.bufferMemory);
    if (result != VK_SUCCESS) {
        ERROR_MSG("Failed to allocate buffer memory!");
        return result;
    }

    result = vkBindBufferMemory(m_device, createdBuffer.buffer, createdBuffer.bufferMemory, 0);
    if (result != VK_SUCCESS) {
        ERROR_MSG("Failed to bind buffer memory!");
        return result;
    }

    createdBuffer.bufferSize = bufferInfo.size;
    createdBuffer.realBufferSize = memReq.size;

    return result;
}

VkResult VULKAN_DRIVER_INTERFACE::CreateAndFillBuffer(const VkBufferCreateInfo& bufferInfo, const uint8_t* pSourceData, bool isUpdatedByCPU, VULKAN_BUFFER& createdBuffer)
{
    VkResult result = CreateBuffer(bufferInfo, isUpdatedByCPU, createdBuffer);
    if (result == VK_SUCCESS) {
        result = FillBuffer(pSourceData, bufferInfo.size, 0, m_intermediateStagingBuffer);
    } 
    if (result == VK_SUCCESS) {
        CopyBuffer(m_intermediateStagingBuffer, createdBuffer, bufferInfo.size);
    }
    return result;
}

size_t VULKAN_DRIVER_INTERFACE::GetDeviceCoherentValue (size_t bufferSize) const
{
    return(bufferSize + m_deviceProperties.limits.nonCoherentAtomSize - 1) & ~(m_deviceProperties.limits.nonCoherentAtomSize - 1);
}

VkResult VULKAN_DRIVER_INTERFACE::FillBuffer (const uint8_t* pSourceData,  uint64_t rawDataSize, uint64_t offset, VULKAN_BUFFER& buffer)
{
    void* data;
    size_t mappedSize = GetDeviceCoherentValue(rawDataSize);
    VkResult result = vkMapMemory(m_device, buffer.bufferMemory, offset, mappedSize, 0, &data);
    if (result != VK_SUCCESS) {
        return result;
    }
    memcpy(data, pSourceData, rawDataSize);

    VkMappedMemoryRange memoryRange{};
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.pNext = nullptr;
    memoryRange.offset = offset;
    memoryRange.memory = buffer.bufferMemory;
    memoryRange.size = mappedSize;
    result = vkFlushMappedMemoryRanges(m_device, 1, &memoryRange);

    vkUnmapMemory(m_device, buffer.bufferMemory);

    return result;
}



void VULKAN_DRIVER_INTERFACE::CopyBuffer(VULKAN_BUFFER srcBuffer, VULKAN_BUFFER dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer.buffer, dstBuffer.buffer, 1, &copyRegion);

    EndSingleTimeCommands(commandBuffer);
}

void VULKAN_DRIVER_INTERFACE::DestroyBuffer(VULKAN_BUFFER& buffer)
{
    vkDestroyBuffer(m_device, buffer.buffer, nullptr);
    vkFreeMemory(m_device, buffer.bufferMemory, nullptr);
}

VkImageAspectFlags CastFormatToAspect(VkFormat format) {
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D16_UNORM_S8_UINT ||
        format == VK_FORMAT_D24_UNORM_S8_UINT ||
        format == VK_FORMAT_D32_SFLOAT_S8_UINT)
    {
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    if(format == VK_FORMAT_D16_UNORM ||
       format == VK_FORMAT_D32_SFLOAT) 
    {
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    return aspect;
}

void VULKAN_DRIVER_INTERFACE::ChangeTextureLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VULKAN_TEXTURE& texture)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture.image;
    barrier.subresourceRange.aspectMask = CastFormatToAspect(texture.format);
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = texture.mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) 
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT; //yep, store of depth on this stage
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
    {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    } else {
        ERROR_MSG("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        m_curCommandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

VkResult VULKAN_DRIVER_INTERFACE::CreateTextureImage(const VkImageCreateInfo& imageInfo, VULKAN_TEXTURE& createdTexture)
{
    VkResult result = vkCreateImage(m_device, &imageInfo, nullptr, &createdTexture.image);
    if (result != VK_SUCCESS) {
        ERROR_MSG( "Can't create texture image");
        return result;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, createdTexture.image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (allocInfo.memoryTypeIndex == -1) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    result = AllocateMemory(allocInfo, createdTexture.imageMemory);
    if(result != VK_SUCCESS) {
        ERROR_MSG("Can't allocate memory for texture");
        return result;
    }

    result = vkBindImageMemory(m_device, createdTexture.image, createdTexture.imageMemory, 0);
    if (result != VK_SUCCESS) {
        ERROR_MSG("Can't bind memory for texture");
        return result;
    }

    createdTexture.width = imageInfo.extent.width;
    createdTexture.height = imageInfo.extent.height;
    createdTexture.format = imageInfo.format;
    createdTexture.mipLevels = imageInfo.mipLevels;

    return result;
}

VkResult VULKAN_DRIVER_INTERFACE::CopyBufferToTexture (const VULKAN_BUFFER& buffer, VkDeviceSize bufferOffset, VULKAN_TEXTURE& texture, uint32_t mipLevel) 
{
    VkBufferImageCopy region{};
    region.bufferOffset = bufferOffset;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        texture.width >> mipLevel,
        texture.height >> mipLevel,
        1
    };

    vkCmdCopyBufferToImage(
        m_curCommandBuffer,
        buffer.buffer,
        texture.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    return VK_SUCCESS;
}

VkResult VULKAN_DRIVER_INTERFACE::CreateImageView(const VkImageViewCreateInfo& imageViewInfo, VULKAN_TEXTURE& texture)
{
    VkResult result = vkCreateImageView(m_device, &imageViewInfo, nullptr, &texture.imageView);
    ASSERT_MSG(result == VK_SUCCESS, "Can't create image view");
    return result;
}


VkResult VULKAN_DRIVER_INTERFACE::CreateRenderTarget(VULKAN_TEXTURE_CREATE_DATA& createData, VULKAN_TEXTURE& createdTexture)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = createData.extent;
    imageInfo.mipLevels = createData.mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.format = createData.format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = createData.usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = 0; // Optional

    VkResult imageCreated = CreateTextureImage(imageInfo, createdTexture);
    if (imageCreated != VK_SUCCESS) {
        return imageCreated;
    }

    //todo: replace CreateImageView inside CreateTexture
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = createdTexture.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = createData.format;;
    viewInfo.subresourceRange.aspectMask = CastFormatToAspect(createData.format);
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkResult imageViewCreated = CreateImageView(viewInfo, createdTexture);
    if (imageViewCreated != VK_SUCCESS) {
        return imageViewCreated;
    }

    return VK_SUCCESS;
}

VkResult VULKAN_DRIVER_INTERFACE::CreateTexture(VULKAN_TEXTURE_CREATE_DATA& createData, VULKAN_TEXTURE& createdTexture)
{
    VkResult bufferFilled = pDrvInterface->FillBuffer(createData.pData, createData.dataSize, 0, m_intermediateStagingBuffer);
    if (bufferFilled != VK_SUCCESS) {
        WARNING_MSG("Can't fill tex buffer\n");
        return bufferFilled;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = createData.extent;
    imageInfo.mipLevels = createData.mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.format = createData.format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = createData.usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = 0; // Optional

    VkResult textureCreated = pDrvInterface->CreateTextureImage(imageInfo, createdTexture);
    if (textureCreated != VK_SUCCESS) {
        return textureCreated;
    }

    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
    SetupCurrentCommandBuffer(commandBuffer);
    ChangeTextureLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, createdTexture);

    for (uint32_t mipLevelId = 0; mipLevelId < createData.mipLevels; mipLevelId++) {
        VkResult textureDataSubmitted = pDrvInterface->CopyBufferToTexture(m_intermediateStagingBuffer, mipLevelId == 0 ? 0 : createData.mipLevelsOffsets[mipLevelId], createdTexture, mipLevelId);
        if (textureDataSubmitted != VK_SUCCESS) {
            WARNING_MSG("Can't submit data to texture\n");
            return textureDataSubmitted;
        }
    }
    ChangeTextureLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, createdTexture);
    EndSingleTimeCommands(commandBuffer);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = createdTexture.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = createData.format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = createData.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkResult imageViewCreated = pDrvInterface->CreateImageView(viewInfo, createdTexture);
    if (imageViewCreated != VK_SUCCESS) {
        WARNING_MSG("Can't create texture image view\n");
        return imageViewCreated;
    }

    return VK_SUCCESS;
}

void VULKAN_DRIVER_INTERFACE::DestroyTexture(VULKAN_TEXTURE& texture) {
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
    SetupCurrentCommandBuffer(commandBuffer);
    ChangeTextureLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture);

    vkDestroyImageView(m_device, texture.imageView, nullptr);
    vkDestroyImage(m_device, texture.image, nullptr);
    vkFreeMemory(m_device, texture.imageMemory, nullptr);
    EndSingleTimeCommands(commandBuffer);
}


VkResult VULKAN_DRIVER_INTERFACE::AllocateMemory (const VkMemoryAllocateInfo& allocationInfo, VkDeviceMemory& allocatedMemory)
{
    VkResult result = vkAllocateMemory(m_device, &allocationInfo, nullptr, &allocatedMemory);
    ASSERT(result == VK_SUCCESS);
    return result;
}

void VULKAN_DRIVER_INTERFACE::FreeMemory (VkDeviceMemory& allocatedMemory)
{
    vkFreeMemory(m_device, allocatedMemory, nullptr);
}


VkResult VULKAN_DRIVER_INTERFACE::CreateDecriptorPools()
{
    //todo : TOO MUCH DESCRIPTORS!
    std::array<VkDescriptorPoolSize, 3> poolSizeDesc;
    poolSizeDesc[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizeDesc[0].descriptorCount = static_cast<uint32_t>(2048);
    poolSizeDesc[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    poolSizeDesc[1].descriptorCount = static_cast<uint32_t>(2048);
    poolSizeDesc[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    poolSizeDesc[2].descriptorCount = static_cast<uint32_t>(4096);

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizeDesc.size());
    poolInfo.pPoolSizes = poolSizeDesc.data();
    poolInfo.maxSets = 4096;
    poolInfo.flags = 0;

    for (int i = 0; i < m_descriptorPool.size(); i++) {
        VkResult result = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool[i]);
        if (result != VK_SUCCESS) {
            return result;
        }
    }
    return VK_SUCCESS;
}

VkResult VULKAN_DRIVER_INTERFACE::CreateDecsriptorSetLayout(uint8_t shaderId)
{
    const std::vector<VkDescriptorSetLayoutBinding>& bindings = pShaderManager->GetDecriptorLayouts(shaderId);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkResult result = vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout[shaderId]);
    return result;
}

VkResult VULKAN_DRIVER_INTERFACE::CreatePiplineLayout(const PIPLINE_LAYOUT_STATE& piplineLayoutKey)
{   
    // push constant using for frequently updated data.
    // it put buffer not in UBO, but directly to command buffer
    VkPushConstantRange pushConstantRange;
    pushConstantRange.offset = 0;
    pushConstantRange.size = 128;
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    bool isShaderUsePushConst = true;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout[piplineLayoutKey.shaderId];
    pipelineLayoutInfo.pushConstantRangeCount = isShaderUsePushConst ? 1 : 0;
    pipelineLayoutInfo.pPushConstantRanges = isShaderUsePushConst ? &pushConstantRange : nullptr;

    VkPipelineLayout pipelineLayout;
    VkResult result = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
    if (result == VK_SUCCESS) {
        m_pipelineLayoutCache.emplace(piplineLayoutKey.GetHashValue(), pipelineLayout);
    } else {
        ERROR_MSG("Can't create pipline layout!");
    }
    return result;
}


VkResult VULKAN_DRIVER_INTERFACE::CreateRenderPass(const RENDER_PASS_STATE& renderPassState)
{
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    std::vector< VkAttachmentDescription> attachmentsDesc;

    uint32_t attachmentId = 0;
    VkAttachmentReference colorAttachmentRef = {};
    VkAttachmentReference depthAttachmentRef = {};

    if (renderPassState.useRT) {
        colorAttachmentRef.attachment = attachmentId++;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        attachmentsDesc.push_back(renderPassState.rtAttachDesc);
    }

    if (renderPassState.useDB) {
        depthAttachmentRef.attachment = attachmentId++;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        attachmentsDesc.push_back(renderPassState.dbAttachDesc);
    }

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentsDesc.size());
    renderPassInfo.pAttachments = attachmentsDesc.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;



    VkRenderPass renderPass;
    VkResult result = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS) {
        ERROR_MSG("Can't create renderPass!");
    }
    m_renderPassCache.emplace(renderPassState.GetHashValue(), renderPass);

    return result;
}


VkResult VULKAN_DRIVER_INTERFACE::InitPipelineState()
{
    CreateDecriptorPools();
    VkResult result;
    for (uint8_t shaderId = 0; shaderId < EFFECT_DATA::SHR_LAST; shaderId++) {
        CreateDecsriptorSetLayout(shaderId);
        PIPLINE_LAYOUT_STATE plk;
        plk.shaderId = shaderId;
        result = CreatePiplineLayout(plk);
        if (result != VK_SUCCESS) {
            return result;
        }
    }
    return VK_SUCCESS;
}

void VULKAN_DRIVER_INTERFACE::StartFrame(double frameStartTime)
{
    m_frameStartTime = frameStartTime;

	vkWaitForFences(m_device, 1, &m_cpuGpuSyncFence[m_curContextId], VK_TRUE, UINT64_MAX);
	vkResetFences(m_device, 1, &m_cpuGpuSyncFence[m_curContextId]);

    VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain.swapChain, UINT64_MAX, m_imageAvailableSemaphore[m_curContextId], VK_NULL_HANDLE, &m_swapChain.curSwapChainImageId);
	ASSERT(result == VK_SUCCESS);

    //todo:
    //vkResetCommandPool(m_device, m_commandPool[m_curContextId], 0);
    VkDescriptorPoolResetFlags resetFlags = 0;
    vkResetDescriptorPool(m_device, m_descriptorPool[m_curContextId], resetFlags);
    pRenderTargetManager->StartFrame();
    pRenderTargetManager->SetCurSwapChainBufferId(m_swapChain.curSwapChainImageId);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr; // Optional

    InvalidateDeviceState();
    SetupCurrentCommandBuffer(m_commandBuffers[m_curContextId]);

    if (vkBeginCommandBuffer(m_curCommandBuffer, &beginInfo) != VK_SUCCESS) {
        ERROR_MSG("Failed to begin recording command buffer!");
    }
}

void VULKAN_DRIVER_INTERFACE::EndFrame(double frameEndTime)
{
    pRenderTargetManager->GetRenderTarget(RT_BACK_BUFFER, 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    pRenderTargetManager->ReturnRenderTarget(RT_BACK_BUFFER);

    if (vkEndCommandBuffer(m_curCommandBuffer) != VK_SUCCESS) {
        ERROR_MSG("Failed to record command buffer!");
    }
    SubmitCommandBuffer();

	VkSemaphore waitSemaphores[] = { m_renderFinishedSemaphore[m_curContextId] };

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = waitSemaphores;

	VkSwapchainKHR swapChains[] = { m_swapChain.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &m_swapChain.curSwapChainImageId;
	presentInfo.pResults = nullptr; // Optional
	vkQueuePresentKHR(m_queueFamilies.presentQueue, &presentInfo);

    m_frameTime = frameEndTime - m_frameStartTime;
	m_frameId++;
	m_curContextId = (m_curContextId + 1) % NUM_FRAME_BUFFERS;
}

void VULKAN_DRIVER_INTERFACE::SubmitCommandBuffer()
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore[m_curContextId] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_curCommandBuffer;

	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore[m_curContextId] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(m_queueFamilies.graphicsQueue, 1, &submitInfo, m_cpuGpuSyncFence[m_curContextId]) != VK_SUCCESS) {
		ERROR_MSG("Failed to submit draw command buffer!");
	}
}

VkResult VULKAN_DRIVER_INTERFACE::CreateFrameBuffer(const FRAME_BUFFER_STATE& frameBufferState)
{
    std::vector<VkImageView> attachments;

    if (frameBufferState.pRenderTargetTexture) {
        attachments.push_back(frameBufferState.pRenderTargetTexture->imageView);
    }
    if (frameBufferState.pDepthTexture) {
        attachments.push_back(frameBufferState.pDepthTexture->imageView);
    }

    VkFramebufferCreateInfo frameBufferInfo = {};
    frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferInfo.flags = 0;
    frameBufferInfo.height = frameBufferState.GetFrameBufferHeight();
    frameBufferInfo.width = frameBufferState.GetFrameBufferWigth();
    frameBufferInfo.layers = 1;
    frameBufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    frameBufferInfo.pAttachments = attachments.data();
    frameBufferInfo.renderPass = m_renderPassCache.find(frameBufferState.renderPassId)->second;

    VkFramebuffer framebuffer;
    VkResult result = vkCreateFramebuffer(m_device, &frameBufferInfo, nullptr, &framebuffer);
    if (result != VK_SUCCESS) {
        ERROR_MSG("Cant' create frame buffer!");
        framebuffer = VK_NULL_HANDLE;
    }
    m_frameBufferCache.emplace(frameBufferState.GetHashValue(), framebuffer);

    return result;
}

void VULKAN_DRIVER_INTERFACE::InvalidateDeviceState()
{
    m_updatePiplineLayout = true;
    m_updatePiplineState = true;
    m_updateDescriptorSet = true;

    m_curPassImageDescriptors.clear();
    m_curPassBufferDescriptors.clear();

    m_isDynamicDepthBiasDirty = false;
    m_isDynamicScissorRectDirty = false;

    //states
    m_curPiplineLayout = VK_NULL_HANDLE;
    m_curPiplineLayoutState = PIPLINE_LAYOUT_STATE();

    m_curRenderPass = VK_NULL_HANDLE;
    m_curRenderPassState = RENDER_PASS_STATE();

    m_curFrameBuffer = VK_NULL_HANDLE;
    m_curFrameBufferState = FRAME_BUFFER_STATE();

    m_curPipeline = VK_NULL_HANDLE;
    m_curPiplineState.piplineLayoutId = SIZE_MAX;
    m_curPiplineState.shaderId = EFFECT_DATA::SHR_LAST;
    m_curPiplineState.vertexFormatId = UINT8_MAX;
    m_curPiplineState.dynamicFlagsBitset.reset();
    m_curPiplineState.depthStateState.depthTestEnable = false;
    m_curPiplineState.depthStateState.depthWriteEnable = false;
    m_curPiplineState.depthStateState.depthCompareOp = false;
    m_curPiplineState.depthStateState.stencilTestEnable = false;
    m_curPiplineState.viewportHeight = 0;
    m_curPiplineState.viewportWidth = 0;

    m_curTextureState.fill(VK_NULL_HANDLE);
    m_curVertexBuffer = VULKAN_BUFFER();
    m_curIndexBuffer = VULKAN_BUFFER();
}


void VULKAN_DRIVER_INTERFACE::SetVertexBuffer(VULKAN_BUFFER vertexBuffer, uint32_t offset)
{
    if (m_curVertexBuffer != vertexBuffer) 
    {
        m_curVertexBuffer = vertexBuffer;
        VkBuffer vertexBuffers[] = { vertexBuffer.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_curCommandBuffer, 0, 1, vertexBuffers, offsets);
    }
}

void VULKAN_DRIVER_INTERFACE::SetIndexBuffer(VULKAN_BUFFER indexBuffer, uint32_t offset)
{
    if (m_curIndexBuffer != indexBuffer)
    {
        m_curIndexBuffer = indexBuffer;
        vkCmdBindIndexBuffer(m_curCommandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
    }
    
}


void VULKAN_DRIVER_INTERFACE::FillPushConstantBuffer(const void* pData, uint32_t dataSize)
{
    ASSERT(dataSize <= m_pushConstantBuffer.size());
    m_pushConstantBufferDirtySize = dataSize;
    std::memcpy(m_pushConstantBuffer.data(), pData, m_pushConstantBufferDirtySize);
}

void VULKAN_DRIVER_INTERFACE::FillConstBuffer(uint32_t bufferId, const void* pData, uint32_t dataSize)
{
    ASSERT(EFFECT_DATA::CONST_BUFFERS_SIZE[bufferId] == dataSize);
    if (m_constBufferOffsets[bufferId] + EFFECT_DATA::CONST_BUFFERS_SIZE[bufferId] > m_constBuffers[bufferId].realBufferSize) {
        m_constBufferOffsets[bufferId] = 0;
    }
    FillBuffer(static_cast<const uint8_t*>(pData), dataSize, m_constBufferOffsets[bufferId], m_constBuffers[bufferId]);
    m_constBufferLastRecordOffset[bufferId] = m_constBufferOffsets[bufferId];
    m_constBufferOffsets[bufferId] += GetDeviceCoherentValue(EFFECT_DATA::CONST_BUFFERS_SIZE[bufferId]);
}

void VULKAN_DRIVER_INTERFACE::SetConstBuffer(uint32_t bufferId)
{
    std::pair<uint8_t, VkDescriptorBufferInfo> bufferInfo;
    bufferInfo.first         = EFFECT_DATA::CONST_BUFFERS_SLOT[bufferId];
    bufferInfo.second.buffer = m_constBuffers[bufferId].buffer;
    bufferInfo.second.offset = m_constBufferLastRecordOffset[bufferId];
    bufferInfo.second.range  = EFFECT_DATA::CONST_BUFFERS_SIZE[bufferId];

    m_updateDescriptorSet = true;
    m_curPassBufferDescriptors.push_back(bufferInfo);
}

void VULKAN_DRIVER_INTERFACE::SetTexture(const VULKAN_TEXTURE* texture, uint32_t slot)
{
//     if (texture == nullptr && m_curTextureState[slot] == VK_NULL_HANDLE) {
//         return;
//     }
//     if ( texture->imageView == m_curTextureState[slot]) {
//         return;
//     }

    m_updateDescriptorSet = true;
    if (texture == nullptr) {
        m_curTextureState[slot] = nullptr;
    } else {
        m_curTextureState[slot] = texture->imageView;

        std::pair<uint8_t, VkDescriptorImageInfo> imageInfo;
        imageInfo.first = slot;
        imageInfo.second.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.second.imageView = texture->imageView;
        imageInfo.second.sampler = nullptr;
        m_curPassImageDescriptors.push_back(imageInfo);
    }
}

void VULKAN_DRIVER_INTERFACE::SetShader(uint8_t shaderId)
{
    if (shaderId != m_curPiplineState.shaderId) {
        m_curPiplineLayoutState.shaderId = shaderId;
        m_curPiplineState.shaderId       = shaderId;

        m_updatePiplineLayout = true;
        m_updatePiplineState = true;
    }
}

void VULKAN_DRIVER_INTERFACE::SetVertexFormat(uint8_t vertexFormat)
{
    if (vertexFormat != m_curPiplineState.vertexFormatId) {
        m_curPiplineState.vertexFormatId = vertexFormat;
        m_updatePiplineState = true;
    }
}

void VULKAN_DRIVER_INTERFACE::SetRenderTarget(RENDER_TARGET rtIndex, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp)
{
    //todo: remove VK_ACCESS_COLOR_ATTACHMENT_READ_BIT when we are drawing without blending
    const VULKAN_TEXTURE* rtTex = pRenderTargetManager->GetRenderTarget(rtIndex, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    m_curRenderPassState.useRT = true;
    m_curRenderPassState.rtAttachDesc.format = rtTex->format;
    m_curRenderPassState.rtAttachDesc.loadOp = loadOp;
    m_curRenderPassState.rtAttachDesc.storeOp = storeOp;

    m_curRenderPassState.rtAttachDesc.flags = 0;
    m_curRenderPassState.rtAttachDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    m_curRenderPassState.rtAttachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_curRenderPassState.rtAttachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_curRenderPassState.rtAttachDesc.initialLayout = pRenderTargetManager->GetRenderTargetPrevLayout(rtIndex);
    m_curRenderPassState.rtAttachDesc.finalLayout = pRenderTargetManager->GetRenderTargetFutureLayout(rtIndex);

    m_curFrameBufferState.pRenderTargetTexture = rtTex;
    m_curFrameBufferState.rtIndex = rtIndex;

}

void VULKAN_DRIVER_INTERFACE::RemoveRenderTarget()
{
    m_curRenderPassState.useRT = false;
    m_curRenderPassState.rtAttachDesc.format =  VK_FORMAT_UNDEFINED;
    m_curRenderPassState.rtAttachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
    m_curRenderPassState.rtAttachDesc.storeOp = VK_ATTACHMENT_STORE_OP_MAX_ENUM;

    m_curFrameBufferState.rtIndex = RT_LAST;
    m_curFrameBufferState.pRenderTargetTexture = VK_NULL_HANDLE;
}

void VULKAN_DRIVER_INTERFACE::SetRenderTargetAsShaderResource(RENDER_TARGET rtIndex, uint32_t slot)
{
    const VULKAN_TEXTURE* rtTex = pRenderTargetManager->GetRenderTarget(rtIndex, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    SetTexture(rtTex, slot);
}

void VULKAN_DRIVER_INTERFACE::SetDepthBuffer(RENDER_TARGET rtIndex, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp)
{
    const VULKAN_TEXTURE* dbTex = pRenderTargetManager->GetRenderTarget(
        rtIndex, 
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );

    m_curRenderPassState.useDB = true;
    m_curRenderPassState.dbAttachDesc.format = dbTex->format;
    m_curRenderPassState.dbAttachDesc.loadOp = loadOp;
    m_curRenderPassState.dbAttachDesc.storeOp = storeOp;

    m_curRenderPassState.dbAttachDesc.flags = 0;
    m_curRenderPassState.dbAttachDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    m_curRenderPassState.dbAttachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_curRenderPassState.dbAttachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_curRenderPassState.dbAttachDesc.initialLayout = pRenderTargetManager->GetRenderTargetPrevLayout(rtIndex);;
    m_curRenderPassState.dbAttachDesc.finalLayout = pRenderTargetManager->GetRenderTargetFutureLayout(rtIndex);;

    m_curFrameBufferState.pDepthTexture = dbTex;
    m_curFrameBufferState.dbIndex = rtIndex;
}

void VULKAN_DRIVER_INTERFACE::RemoveDepthBuffer()
{
    m_curRenderPassState.useDB = false;
    m_curRenderPassState.dbAttachDesc.format = VK_FORMAT_UNDEFINED;
    m_curRenderPassState.dbAttachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
    m_curRenderPassState.dbAttachDesc.storeOp = VK_ATTACHMENT_STORE_OP_MAX_ENUM;

    m_curFrameBufferState.pDepthTexture = VK_NULL_HANDLE;
    m_curFrameBufferState.dbIndex = RT_LAST;
}

void VULKAN_DRIVER_INTERFACE::SetDynamicState(VkDynamicState state, bool enableState)
{
    ASSERT(state < 8);
    const uint8_t stateMask = 1 << state;
    if (m_curPiplineState.dynamicFlagsBitset.test(state) != enableState) {
        m_curPiplineState.dynamicFlagsBitset.set(state, enableState);
        m_updatePiplineState = true;
    }
}

void VULKAN_DRIVER_INTERFACE::SetDepthTestState(bool depthTestEnable)
{
    if (m_curPiplineState.depthStateState.depthTestEnable != depthTestEnable) {
        m_curPiplineState.depthStateState.depthTestEnable = depthTestEnable;
        m_updatePiplineState = true;
    }
}

void VULKAN_DRIVER_INTERFACE::SetDepthWriteState(bool depthWriteEnable)
{
    if (m_curPiplineState.depthStateState.depthWriteEnable != depthWriteEnable) {
        m_curPiplineState.depthStateState.depthWriteEnable = depthWriteEnable;
        m_updatePiplineState = true;
    }
}

void VULKAN_DRIVER_INTERFACE::SetDepthComparitionOperation(bool depthCompare)
{
    if (m_curPiplineState.depthStateState.depthCompareOp != depthCompare) {
        m_curPiplineState.depthStateState.depthCompareOp = depthCompare;
        m_updatePiplineState = true;
    }
}

void VULKAN_DRIVER_INTERFACE::SetStencilTestState(bool stencilTestEnable)
{
    if (m_curPiplineState.depthStateState.stencilTestEnable != stencilTestEnable) {
        m_curPiplineState.depthStateState.stencilTestEnable = stencilTestEnable;
        m_updatePiplineState = true;
    }
}

void VULKAN_DRIVER_INTERFACE::SetScissorRect(const VkRect2D& scissorRect) 
{
    m_dynamicScissorRect = scissorRect;
    m_isDynamicScissorRectDirty = true;
}

void VULKAN_DRIVER_INTERFACE::SetDepthBiasParams(float depthBiasConstant, float depthBiasSlope)
{
    m_dynamicBiasParams = {depthBiasConstant, depthBiasSlope};
    m_isDynamicDepthBiasDirty = true;
}

void VULKAN_DRIVER_INTERFACE::BeginRenderPass()
{
    const size_t curRenderPassId = m_curRenderPassState.GetHashValue();
    auto renderPass = m_renderPassCache.find(curRenderPassId);
    if (renderPass == m_renderPassCache.end()) {
        CreateRenderPass(m_curRenderPassState);
        renderPass = m_renderPassCache.find(curRenderPassId);
    }
    m_curRenderPass = renderPass->second;
    m_curFrameBufferState.renderPassId = curRenderPassId;

    if (m_curPiplineState.renderPassId != curRenderPassId) {
        m_curPiplineState.renderPassId = curRenderPassId;
        m_updatePiplineState = true;
    }

    const size_t curFrameBufferId = m_curFrameBufferState.GetHashValue();
    auto frameBuffer = m_frameBufferCache.find(curFrameBufferId);
    if (frameBuffer == m_frameBufferCache.end()) {
        CreateFrameBuffer(m_curFrameBufferState);
        frameBuffer = m_frameBufferCache.find(curFrameBufferId);
    }
    m_curFrameBuffer = frameBuffer->second;

    if (m_curPiplineState.viewportHeight != m_curFrameBufferState.GetFrameBufferHeight()
        || m_curPiplineState.viewportWidth != m_curFrameBufferState.GetFrameBufferWigth()) 
    {
        m_curPiplineState.viewportHeight = m_curFrameBufferState.GetFrameBufferHeight();
        m_curPiplineState.viewportWidth = m_curFrameBufferState.GetFrameBufferWigth();
        m_updatePiplineState = true;
    }

    uint32_t numClearValues = 0;
    std::array<VkClearValue, 2> clearValues{};
    if (m_curRenderPassState.useRT) {
        clearValues[numClearValues++].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    }
    if (m_curRenderPassState.useDB) {
        clearValues[numClearValues++].depthStencil = { 1.0f, 0 };
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_curRenderPass;
    renderPassInfo.framebuffer = m_curFrameBuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent.height = m_curFrameBufferState.GetFrameBufferHeight();
    renderPassInfo.renderArea.extent.width = m_curFrameBufferState.GetFrameBufferWigth();
    renderPassInfo.clearValueCount = numClearValues;
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(m_curCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VULKAN_DRIVER_INTERFACE::EndRenderPass()
{
    vkCmdEndRenderPass(m_curCommandBuffer);
    RemoveRenderTarget();
    RemoveDepthBuffer();
    InvalidateDeviceState();
    pRenderTargetManager->ReturnAllRenderTargets();
}

void VULKAN_DRIVER_INTERFACE::Draw(uint32_t vertexesNum)
{
    const bool stateUpdated = UpdatePiplineState();
    if (stateUpdated) {
        vkCmdDraw(m_curCommandBuffer, vertexesNum, 1, 0, 0);
    }
}

void VULKAN_DRIVER_INTERFACE::DrawIndexed(size_t indexesNum, size_t vertexBufferOffset, size_t indexBufferOffset)
{
    const bool stateUpdated = UpdatePiplineState();
    if (stateUpdated) {
        vkCmdDrawIndexed(m_curCommandBuffer, indexesNum, 1, indexBufferOffset, vertexBufferOffset, 0);
    }
}

void VULKAN_DRIVER_INTERFACE::DrawFullscreen()
{
    Draw(3);
}


bool VULKAN_DRIVER_INTERFACE::UpdatePiplineState()
{
    bool bindDescriptorSet = m_updateDescriptorSet;
    if (m_updatePiplineLayout) {
        const size_t curLayoutId = m_curPiplineLayoutState.GetHashValue();
        auto piplineLayout = m_pipelineLayoutCache.find(curLayoutId);
        if (piplineLayout == m_pipelineLayoutCache.end()) {
            CreatePiplineLayout(m_curPiplineLayoutState);
            piplineLayout = m_pipelineLayoutCache.find(curLayoutId);
        }
        m_curPiplineLayout = piplineLayout->second;
        if (m_curPiplineState.piplineLayoutId != curLayoutId) {
            m_curPiplineState.piplineLayoutId = curLayoutId;
            m_updatePiplineState = true;
            bindDescriptorSet = true;
        }
        
        m_updatePiplineLayout = false;
    }

    if (m_updateDescriptorSet) {
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool[m_curContextId];
        allocInfo.descriptorSetCount  = 1;
        allocInfo.pSetLayouts = &m_descriptorSetLayout[m_curPiplineLayoutState.shaderId];
        VkResult result = vkAllocateDescriptorSets(m_device, &allocInfo, &m_curDescriptorSet);

        std::vector<VkWriteDescriptorSet> writeDescSet;
        for (const auto& samplerDesc : m_samplerDescriptors) {
            VkWriteDescriptorSet writeDesc = {};
            writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDesc.dstSet = m_curDescriptorSet;
            writeDesc.dstBinding = samplerDesc.first;
            writeDesc.dstArrayElement = 0;
            writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            writeDesc.descriptorCount = 1;
            writeDesc.pImageInfo = &samplerDesc.second;

            writeDescSet.push_back(writeDesc);
        }
        for (const auto& imageDesc : m_curPassImageDescriptors) {
            VkWriteDescriptorSet writeDesc = {};
            writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDesc.dstSet = m_curDescriptorSet;
            writeDesc.dstBinding = imageDesc.first;
            writeDesc.dstArrayElement = 0;
            writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            writeDesc.descriptorCount = 1;
            writeDesc.pImageInfo = &imageDesc.second;

            writeDescSet.push_back(writeDesc);
        }
        for (const auto& bufferDesc : m_curPassBufferDescriptors) {
            VkWriteDescriptorSet writeDesc = {};
            writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDesc.dstSet = m_curDescriptorSet;
            writeDesc.dstBinding = bufferDesc.first;
            writeDesc.dstArrayElement = 0;
            writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDesc.descriptorCount = 1;
            writeDesc.pBufferInfo = &bufferDesc.second;

            writeDescSet.push_back(writeDesc);
        }
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeDescSet.size()), writeDescSet.data(), 0, nullptr);
        m_curPassImageDescriptors.clear();
        m_curPassBufferDescriptors.clear();
        m_updateDescriptorSet = false;
    }

    if (bindDescriptorSet) {
        vkCmdBindDescriptorSets(m_curCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_curPiplineLayout, 0, 1, &m_curDescriptorSet, 0, nullptr);
    }

    if (m_updatePiplineState) {
        const size_t curPiplineStateObjectId = m_curPiplineState.GetHashValue();
        auto piplineState = m_pipelineStateCache.find(curPiplineStateObjectId);
        if (piplineState == m_pipelineStateCache.end()) {
            VkResult piplineCreated = CreateGraphicPipeline(m_curPiplineState);
            ASSERT_MSG(piplineCreated == VK_SUCCESS, "Pipine wasn't created!");
            piplineState = m_pipelineStateCache.find(curPiplineStateObjectId);
        }
        if (piplineState->second == VK_NULL_HANDLE) {
            return false;
        }
        m_curPipeline = piplineState->second;
        vkCmdBindPipeline(m_curCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_curPipeline);
        m_updatePiplineState = false;
    }

    if (m_pushConstantBufferDirtySize) {
        vkCmdPushConstants(
            m_curCommandBuffer,
            m_curPiplineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            m_pushConstantBufferDirtySize,
            m_pushConstantBuffer.data()
        );
        m_pushConstantBufferDirtySize = 0;
    }
    if (m_isDynamicScissorRectDirty) {
        vkCmdSetScissor(m_curCommandBuffer, 0, 1, &m_dynamicScissorRect);
        m_isDynamicScissorRectDirty = false;
    }

    if (m_isDynamicDepthBiasDirty) {
        //we can't change clamp value dynamically by documentation
        vkCmdSetDepthBias(m_curCommandBuffer, m_dynamicBiasParams.x, 0.f, m_dynamicBiasParams.y);
        m_isDynamicDepthBiasDirty = false;
    }

    return true;
}

VkResult VULKAN_DRIVER_INTERFACE::CreateGraphicPipeline(const PIPLINE_STATE& piplineState)
{
    ASSERT(m_pipelineLayoutCache.find(piplineState.piplineLayoutId) != m_pipelineLayoutCache.end());

    VERTEX_FORMAT_DESCRIPTOR vertexFormatDescriptor = pVertexDeclarationManager->GetDesc(piplineState.vertexFormatId);
    VkPipelineVertexInputStateCreateInfo vertexInputDescriptor;
    vertexInputDescriptor.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputDescriptor.flags = 0;
    vertexInputDescriptor.pNext = nullptr;
    vertexInputDescriptor.pVertexBindingDescriptions = vertexFormatDescriptor.bindingDesctiption.data();
    vertexInputDescriptor.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexFormatDescriptor.bindingDesctiption.size());
    vertexInputDescriptor.pVertexAttributeDescriptions = vertexFormatDescriptor.attributeDescription.data();
    vertexInputDescriptor.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexFormatDescriptor.attributeDescription.size());

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)piplineState.viewportWidth;
    viewport.height = (float)piplineState.viewportHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { piplineState.viewportWidth , piplineState.viewportHeight };

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    rasterizer.depthBiasEnable = piplineState.dynamicFlagsBitset.test(VK_DYNAMIC_STATE_DEPTH_BIAS);
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = rasterizer.depthBiasEnable ? DEPTH_BIAS_CLAMP : 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = piplineState.depthStateState.depthTestEnable;
    depthStencil.depthWriteEnable = piplineState.depthStateState.depthWriteEnable;
    depthStencil.depthCompareOp = piplineState.depthStateState.depthCompareOp ? VK_COMPARE_OP_LESS : VK_COMPARE_OP_ALWAYS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = piplineState.depthStateState.stencilTestEnable;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional
    
    //todo
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    if (piplineState.shaderId != EFFECT_DATA::SHR_UI) {
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
    } else {
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional
   
    std::vector<VkDynamicState> dynamicStateFlags;
    if (piplineState.dynamicFlagsBitset.any()) {
        for (int stateId = 0; stateId < piplineState.dynamicFlagsBitset.size(); stateId++) {
            if(piplineState.dynamicFlagsBitset.test(stateId)) {
                dynamicStateFlags.push_back(VkDynamicState(stateId));
            }
        }
    }


    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.flags = 0;
    dynamicState.pNext = nullptr;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateFlags.size());
    dynamicState.pDynamicStates = dynamicStateFlags.data();

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {};
    uint32_t numStages = 0;
    const VkShaderModule& vertexShader = pShaderManager->GetVertexShader(piplineState.shaderId);
    if (vertexShader) {
        VkPipelineShaderStageCreateInfo& vertexShaderStageInfo = shaderStages[numStages++];
        vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = vertexShader;
        vertexShaderStageInfo.flags = 0;
        vertexShaderStageInfo.pName = "main";
    }

    const VkShaderModule& pixelShader = pShaderManager->GetPixelShader(piplineState.shaderId);
    if (pixelShader) {
        VkPipelineShaderStageCreateInfo& pixelShaderStageInfo = shaderStages[numStages++];
        pixelShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pixelShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pixelShaderStageInfo.module = pixelShader;
        pixelShaderStageInfo.flags = 0;
        pixelShaderStageInfo.pName = "main";
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = numStages;
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputDescriptor;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayoutCache[piplineState.piplineLayoutId];
    pipelineInfo.renderPass = m_renderPassCache[piplineState.renderPassId];
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    VkPipeline pipline;
    VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipline);
    if (result != VK_SUCCESS) {
        ERROR_MSG("Can't create pipline!");
        vkDestroyPipeline(m_device, pipline, nullptr);
        pipline = VK_NULL_HANDLE;
    } 
    m_pipelineStateCache.emplace(piplineState.GetHashValue(), pipline);

    return result;
}

std::vector<const char*> VULKAN_DRIVER_INTERFACE::GetRequiredInstanceExtentions() const
{
    std::vector<const char*> requiredExtentions;
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        requiredExtentions.push_back(std::move(glfwExtensions[i]));
    }

    if (m_enableValidationLayer) {
        requiredExtentions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        for (const char* requiredExtentions : requiredExtentions) {
            bool isExtantionAvailable = false;
            for (const VkExtensionProperties& avalanbleExtention : extensions) {
                if (strcmp(avalanbleExtention.extensionName, requiredExtentions) == 0) {
                    isExtantionAvailable = true;
                    break;
                }
            }
            assert(isExtantionAvailable);
        }
    }
    return requiredExtentions;
}

std::vector<const char*> VULKAN_DRIVER_INTERFACE::GetRequiredInstanceLayers() const
{
    std::vector<const char*> requiredLayers;
    if (m_enableValidationLayer) {
        requiredLayers.push_back("VK_LAYER_LUNARG_standard_validation");

        //check
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* requiredLayer : requiredLayers) {
            bool isLayerAvailable = false;

            for (const VkLayerProperties& layerProperties : availableLayers) {
                if (strcmp(layerProperties.layerName, requiredLayer) == 0) {
                    isLayerAvailable = true;
                    break;
                }
            }
            assert(isLayerAvailable);
        }
    }
    return requiredLayers;
}

std::vector<const char*> VULKAN_DRIVER_INTERFACE::GetRequiredDeviceExtentions() const
{
    std::vector<const char*> requiredExtentions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    return requiredExtentions;
}

VkResult VULKAN_DRIVER_INTERFACE::InitInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> enabledExtentions = GetRequiredInstanceExtentions();
    std::vector<const char*> enabledLayers = GetRequiredInstanceLayers();

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtentions.size());
    createInfo.ppEnabledExtensionNames = enabledExtentions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    createInfo.ppEnabledLayerNames = enabledLayers.data();

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    return result;
}

VkResult VULKAN_DRIVER_INTERFACE::InitPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    int curDeviceRating = 0;
    for (VkPhysicalDevice& availableDevice : devices) {
        int deviceRate = DeviceSuitabilityRate(availableDevice);
        if (curDeviceRating < deviceRate) {
            m_physicalDevice = availableDevice;
            curDeviceRating = deviceRate;
        }
    }

    if (m_physicalDevice != VK_NULL_HANDLE) {
        m_queueFamilies.createParams = FindQueueFamilies(m_physicalDevice);
        m_swapChain.createParams = GetSwapChainCreateParams(m_physicalDevice);

        vkGetPhysicalDeviceProperties(m_physicalDevice, &m_deviceProperties);
        vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_deviceFeatures);
        return VK_SUCCESS;
    } else {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
}

VkResult VULKAN_DRIVER_INTERFACE::InitLogicalDevice()
{
    float queuePriorities = 1.f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { 
        m_queueFamilies.createParams.graphicsFamilyIndex.value(), 
        m_queueFamilies.createParams.presentFamilyIndex.value() 
    };
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriorities;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    std::vector<const char*> deviceExtentions = GetRequiredDeviceExtentions();
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.ppEnabledExtensionNames = deviceExtentions.data();
    createInfo.enabledExtensionCount = (uint32_t)deviceExtentions.size();

    VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
    if (result == VK_SUCCESS) {
        vkGetDeviceQueue(m_device, m_queueFamilies.createParams.graphicsFamilyIndex.value(), 0, &m_queueFamilies.graphicsQueue);
        vkGetDeviceQueue(m_device, m_queueFamilies.createParams.presentFamilyIndex.value(), 0, &m_queueFamilies.presentQueue);
    }
    return result;
}


VkResult VULKAN_DRIVER_INTERFACE::InitSurface()
{
    uint32_t windowWidth, windowHeight;
    void* pWindow;
    pCallbackGetWindowParams(windowWidth, windowHeight, pWindow);
    VkResult result = glfwCreateWindowSurface(m_instance, (GLFWwindow*)pWindow, nullptr, &m_windowSurface);
    return result;
}

VkResult VULKAN_DRIVER_INTERFACE::InitSwapChain()
{
    const SWAP_CHAIN::SWAP_CHAIN_CREATE_PARAMS& swapChainCreateParams = m_swapChain.createParams;
    uint32_t imageCount = std::clamp(NUM_FRAME_BUFFERS, swapChainCreateParams.minImageCount, swapChainCreateParams.maxImageCount);
	if (imageCount != NUM_FRAME_BUFFERS) {
		ERROR_MSG("Dubble buffering doesn't supported!");
		return VK_ERROR_FORMAT_NOT_SUPPORTED;
	}

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_windowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = swapChainCreateParams.surfaceFormat.format;
    createInfo.imageColorSpace = swapChainCreateParams.surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainCreateParams.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (m_queueFamilies.createParams.graphicsFamilyIndex != m_queueFamilies.createParams.presentFamilyIndex) {
        uint32_t queueFamilyIndices[] = { m_queueFamilies.createParams.graphicsFamilyIndex.value(), m_queueFamilies.createParams.presentFamilyIndex.value() };
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = swapChainCreateParams.presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain.swapChain);
    return result;
}

VkResult VULKAN_DRIVER_INTERFACE::InitSwapChainImages()
{
    VkResult result = VK_SUCCESS;
    uint32_t imageCount;
    std::array<VkImage, NUM_FRAME_BUFFERS> swapChainImages;
    vkGetSwapchainImagesKHR(m_device, m_swapChain.swapChain, &imageCount, nullptr);
    ASSERT(imageCount == NUM_FRAME_BUFFERS);
    vkGetSwapchainImagesKHR(m_device, m_swapChain.swapChain, &imageCount, swapChainImages.data());

    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = m_swapChain.createParams.surfaceFormat.format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VULKAN_TEXTURE swapChainTexture;
    swapChainTexture.format = m_swapChain.createParams.surfaceFormat.format;
    swapChainTexture.height = m_swapChain.createParams.extent.height;
    swapChainTexture.width = m_swapChain.createParams.extent.width;
    swapChainTexture.mipLevels = 1;
    for (uint32_t i = 0; i < imageCount; i++) {
        createInfo.image = swapChainImages[i];
        swapChainTexture.image = swapChainImages[i];
        result = CreateImageView(createInfo, swapChainTexture);
        if (result != VK_SUCCESS) {
            return result;
        }
        pRenderTargetManager->SetupSwapChainTexture(swapChainTexture, i);
    }
    return result;
}


VkResult VULKAN_DRIVER_INTERFACE::InitCommandBuffers()
{
    VkResult result;
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_queueFamilies.createParams.graphicsFamilyIndex.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
    if(result != VK_SUCCESS) {
        return result;
    }

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = NUM_FRAME_BUFFERS;

    result = vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers);

    return VK_SUCCESS;
}

VkResult VULKAN_DRIVER_INTERFACE::InitSemaphoresAndFences()
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (int i = 0; i < NUM_FRAME_BUFFERS; i++) {
		if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore[i]) != VK_SUCCESS) {
			ERROR_MSG("Failed to create semaphores!");
			return VK_INCOMPLETE;
		}
	}

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (int i = 0; i < NUM_FRAME_BUFFERS; i++) {
		if (vkCreateFence(m_device, &fenceInfo, nullptr, &m_cpuGpuSyncFence[i]) != VK_SUCCESS) {
			ERROR_MSG("Failed to create fences!");
			return VK_INCOMPLETE;
		}
	}
    return VK_SUCCESS;
}

VkResult VULKAN_DRIVER_INTERFACE::InitIntermediateBuffers()
{
    VkBufferCreateInfo stagingBufferInfo = {};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = 8192 * 8192 *  sizeof(float);
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult bufferCreated = pDrvInterface->CreateBuffer(stagingBufferInfo, true, m_intermediateStagingBuffer);
    return bufferCreated;
}


VkResult VULKAN_DRIVER_INTERFACE::InitConstBuffers()
{
    for (int i = 0; i < EFFECT_DATA::CB_LAST; i++) {
        ASSERT(NUM_CONSTANT_BUFFERS > EFFECT_DATA::CONST_BUFFERS_SLOT[i]);
    }
    
    m_constBufferOffsets.fill(0);
    m_constBufferLastRecordOffset.fill(0);

    const uint32_t DYNAMIC_CONST_BUFFER_SIZE = 1 * 1024 * 1024; // 1 Mb

    VkBufferCreateInfo constBufferInfo = {};
    constBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    constBufferInfo.size = DYNAMIC_CONST_BUFFER_SIZE;
    constBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    constBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    for (int i = 0; i < m_constBuffers.size(); i++) {
        VkResult bufferCreateStatus = CreateBuffer(constBufferInfo, true, m_constBuffers[i]);
        if (bufferCreateStatus != VK_SUCCESS) {
            return bufferCreateStatus;
        }
    }
    return VK_SUCCESS;
}

void VULKAN_DRIVER_INTERFACE::TermConstBuffers()
{
    for (int i = 0; i < m_constBuffers.size(); i++) {
        DestroyBuffer(m_constBuffers[i]);
    }
}


void VULKAN_DRIVER_INTERFACE::TermIntermediateBuffers() {
    DestroyBuffer(m_intermediateStagingBuffer);
}

void VULKAN_DRIVER_INTERFACE::SetupSamples()
{
    const int START_SAMPLER_INDEX = 120;
    for (int s = 0; s < m_samplers.size(); s++) {

    }
}

VkResult VULKAN_DRIVER_INTERFACE::CreateSampler(VkFilter minMagFilter, VkSamplerMipmapMode mapFilter, VkSamplerAddressMode addressMode, VkCompareOp compareOp, float anisoParam, VkSampler& sampler)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = minMagFilter;
    samplerInfo.minFilter = minMagFilter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = anisoParam > 0.f;
    samplerInfo.maxAnisotropy = anisoParam;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = compareOp == VK_COMPARE_OP_ALWAYS ? VK_FALSE : TRUE;
    samplerInfo.compareOp = compareOp;
    samplerInfo.mipmapMode = mapFilter;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 16.0f;

    VkResult result = vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler);
    ASSERT_MSG(result == VK_SUCCESS, "Can't create sampler!");
    return result;
}

void VULKAN_DRIVER_INTERFACE::DestroySampler(VkSampler& sampler)
{
    vkDestroySampler(m_device, sampler, nullptr);
}

VkResult VULKAN_DRIVER_INTERFACE::InitSamplers()
{
    ASSERT(NUM_SAMPLERS < 8 && NUM_SAMPLERS == EFFECT_DATA::SAMPLER_LAST);
    CreateSampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_COMPARE_OP_ALWAYS, 16.f, m_samplers[EFFECT_DATA::SAMPLER_REPEAT_LINEAR_ANISO]);
    CreateSampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_COMPARE_OP_ALWAYS, 0.f, m_samplers[EFFECT_DATA::SAMPLER_REPEAT_LINEAR]);
    CreateSampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_COMPARE_OP_ALWAYS, 0.f, m_samplers[EFFECT_DATA::SAMPLER_CLAMP_LINEAR]);
    CreateSampler(VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_COMPARE_OP_ALWAYS, 0.f, m_samplers[EFFECT_DATA::SAMPLER_CLAMP_POINT]);
    CreateSampler(VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_COMPARE_OP_LESS, 0.f, m_samplers[EFFECT_DATA::SAMPLER_CLAMP_POINT_CMP]);

    const int FIRST_SAMPLER_BINDING_SLOT = 120;
    for (int i = 0; i < NUM_SAMPLERS; i++) {
        m_samplerDescriptors[i].first = FIRST_SAMPLER_BINDING_SLOT + i;
        m_samplerDescriptors[i].second.sampler = m_samplers[i];
    }
    return VK_SUCCESS;
}

void VULKAN_DRIVER_INTERFACE::TermSamplers()
{
    for (auto& sampler : m_samplers) {
        DestroySampler(sampler);
    }
}


SWAP_CHAIN::SWAP_CHAIN_CREATE_PARAMS VULKAN_DRIVER_INTERFACE::GetSwapChainCreateParams(const VkPhysicalDevice& device) const {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_windowSurface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_windowSurface, &formatCount, nullptr);
    if (formatCount != 0) {
        formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_windowSurface, &formatCount, formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_windowSurface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_windowSurface, &presentModeCount, presentModes.data());
    }

    SWAP_CHAIN::SWAP_CHAIN_CREATE_PARAMS details;
    if (!formats.empty() && !presentModes.empty()) {
        details.ChooseExtent(capabilities);
        details.ChooseSurfaceFormat(formats);
        details.ChooseSwapPresentMode(presentModes);
        details.minImageCount = capabilities.minImageCount;
        details.maxImageCount = capabilities.maxImageCount;
    }

    return details;
}

int VULKAN_DRIVER_INTERFACE::DeviceSuitabilityRate(const VkPhysicalDevice & device) const
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int deviceRate = 0;

    //check features
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU || deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
        deviceRate += 10;
    } else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        deviceRate += 100;
    }
    if (!deviceFeatures.tessellationShader) {
        deviceRate = 0;
    }

    //check is device support necessary queue families
    QUEUE_FAMILIES::QUEUE_FAMILY_CREATE_PARAMS queueFamIndices = FindQueueFamilies(device);
    if (!queueFamIndices.IsComplete()) {
        deviceRate = 0;
    } else if (queueFamIndices.graphicsFamilyIndex == queueFamIndices.presentFamilyIndex) {
        deviceRate += 10;
    }

    //check is device support necessary extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    std::vector<const char*> deviceRequiredExtentions = GetRequiredDeviceExtentions();
    for (const char* reqExtention : deviceRequiredExtentions) {
        bool extentonSupported = false;
        for (const VkExtensionProperties& availableExtention : availableExtensions) {
            if (strcmp(reqExtention, availableExtention.extensionName) == 0) {
                extentonSupported = true;
                break;
            }
        }
        if (!extentonSupported) {
            deviceRate = 0;
        }
    }

    //check swapChain support
    SWAP_CHAIN::SWAP_CHAIN_CREATE_PARAMS scSupport = GetSwapChainCreateParams(device);
    if (scSupport.presentMode == VK_PRESENT_MODE_MAX_ENUM_KHR) {
        deviceRate = 0;
    }
    return deviceRate;
}

QUEUE_FAMILIES::QUEUE_FAMILY_CREATE_PARAMS VULKAN_DRIVER_INTERFACE::FindQueueFamilies(const VkPhysicalDevice & device) const
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    QUEUE_FAMILIES::QUEUE_FAMILY_CREATE_PARAMS indices;
    int familyIndex = 0;
    for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
        if (queueFamily.queueCount == 0) {
            continue;
        }
        VkBool32 isGraphicsSupport = queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT;
        VkBool32 isPresentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, familyIndex, m_windowSurface, &isPresentSupport);

        if (isGraphicsSupport && isPresentSupport) {
            //same family == best performance
            indices.graphicsFamilyIndex = familyIndex;
            indices.presentFamilyIndex = familyIndex;
        }
        if (isGraphicsSupport && !indices.graphicsFamilyIndex.has_value()) {
            indices.graphicsFamilyIndex = familyIndex;
        }
        if (isPresentSupport && !indices.presentFamilyIndex.has_value()) {
            indices.presentFamilyIndex = familyIndex;
        }
        familyIndex++;
    }
    return indices;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    const char* msgType;
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        msgType = "Diagnostic";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        msgType = "Information";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        msgType = "Warning";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        msgType = "Error";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
    default:
        msgType = "";
        break;
    }

    const bool printInfoMsgs = false;
    if (printInfoMsgs || messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        DEBUG_MSG(formatString("%s VALIDATION LAYER message: %s\n", msgType, pCallbackData->pMessage).c_str());
    }

    return VK_FALSE;
}

VkResult VULKAN_DRIVER_INTERFACE::InitDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;

    VkResult result;
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(m_instance, &createInfo, nullptr, &m_debugMessenger);
        result = VK_SUCCESS;
    } else {
        result = VK_ERROR_INITIALIZATION_FAILED;
    }
    return result;
}

// VkResult VULKAN_DRIVER_INTERFACE::InitDefaultRenderPass()
// {
//     RENDER_PASS_STATE state;
//     state.useRT = true;
//     state.useDB = false;
//     state.rtAttachDesc.flags = 0;
//     state.rtAttachDesc.format = m_swapChain.swapChainTexture[0].format;
//     state.rtAttachDesc.samples = VK_SAMPLE_COUNT_1_BIT;
//     state.rtAttachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//     state.rtAttachDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//     state.rtAttachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//     state.rtAttachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//     state.rtAttachDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//     state.rtAttachDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
// 
//     VkResult result = CreateRenderPass(state);
//     m_defaultRenderPassHashValue = m_renderPassCache.find(state.GetHashValue())->first;
//     return result;
// }

void VULKAN_DRIVER_INTERFACE::TermDebugMessenger()
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(m_instance, m_debugMessenger, nullptr);
    }
}

void SWAP_CHAIN::SWAP_CHAIN_CREATE_PARAMS::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        return;
    }
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            return;
        }
    }
    surfaceFormat = availableFormats[0];
}

void SWAP_CHAIN::SWAP_CHAIN_CREATE_PARAMS::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    presentMode = VK_PRESENT_MODE_FIFO_KHR;
}

void SWAP_CHAIN::SWAP_CHAIN_CREATE_PARAMS::ChooseExtent(const VkSurfaceCapabilitiesKHR & capabilities)
{
    uint32_t windowWidth, windowHeight;
    void* pWindow;
    pCallbackGetWindowParams(windowWidth, windowHeight, pWindow);
    extent.width = std::clamp(windowWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(windowHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
}


void VULKAN_DRIVER_INTERFACE::SetupCurrentCommandBuffer(VkCommandBuffer newCurrentBuffer)
{
    m_curCommandBuffer = newCurrentBuffer;
}

VkCommandBuffer VULKAN_DRIVER_INTERFACE::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void VULKAN_DRIVER_INTERFACE::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_queueFamilies.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    //todo: remove idle
    vkQueueWaitIdle(m_queueFamilies.graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

uint32_t FRAME_BUFFER_STATE::GetFrameBufferHeight() const
{
    uint32_t frameBufferHeight = 0;
    if (pRenderTargetTexture) {
        frameBufferHeight = pRenderTargetTexture->height;
    } else if (pDepthTexture) {
        frameBufferHeight = pDepthTexture->height;
    }
    return frameBufferHeight;
}

uint32_t FRAME_BUFFER_STATE::GetFrameBufferWigth() const
{
    uint32_t frameBufferWidth = 0;
    if (pRenderTargetTexture) {
        frameBufferWidth = pRenderTargetTexture->width;
    } else if (pDepthTexture) {
        frameBufferWidth = pDepthTexture->width;
    }
    return frameBufferWidth;
}
