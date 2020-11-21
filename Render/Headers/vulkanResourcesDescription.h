#pragma once
#include "vulkan/vulkan.h"
#include <vector>

struct VULKAN_TEXTURE_CREATE_DATA
{
    VULKAN_TEXTURE_CREATE_DATA() : extent{ 0, 0, 0 }, mipLevels(0), format(VK_FORMAT_UNDEFINED), usage(VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM), pData(nullptr), dataSize(0) {}
    VULKAN_TEXTURE_CREATE_DATA(VkFormat _format, VkImageUsageFlagBits _usage, uint32_t _width, uint32_t _height, uint32_t _depth = 1)
        : extent{ _width, _height, _depth }, mipLevels(1), format(_format), usage(_usage), pData(nullptr), dataSize(0) {}

    const uint8_t* pData;
    size_t dataSize;
    VkExtent3D extent;
    VkFormat format;
    uint32_t mipLevels;
    std::vector<size_t> mipLevelsOffsets;
    VkImageUsageFlagBits usage;
};

struct VULKAN_TEXTURE
{
    VULKAN_TEXTURE() : width(0), height(0), format(VK_FORMAT_UNDEFINED), mipLevels(0), image(VK_NULL_HANDLE), imageMemory(VK_NULL_HANDLE), imageView(VK_NULL_HANDLE) {}

    uint32_t width;
    uint32_t height;
    VkFormat format;
    uint8_t mipLevels;

    VkImage image;
    VkImageView imageView;
    VkDeviceMemory imageMemory;
};

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

struct VULKAN_MESH
{
    uint8_t vertexFormatId;
    VULKAN_BUFFER vertexBuffer;
    VULKAN_BUFFER indexBuffer;
    size_t numOfVertexes;
    size_t numOfIndexes;
};