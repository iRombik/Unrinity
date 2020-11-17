#include <gli.hpp>

#include "textureManager.h"
#include "vulkanDriver.h"
#include "support.h"


std::unique_ptr<TEXTURE_MANAGER> pTextureManager;

void TEXTURE_MANAGER::Init()
{
    m_textureMap.clear();
}

void TEXTURE_MANAGER::LoadCommonTextures()
{
    CreateDefalutTextures();

    LoadTexture("Metal002/Metal002_2K_Color.jpg");
    LoadTexture("Metal002/Metal002_2K_Normal.jpg");
    LoadTexture("Metal002/Metal002_2K_Displacement.jpg");
    LoadTexture("Metal002/Metal002_2K_Roughness.jpg");
    LoadTexture("Metal002/Metal002_2K_Metalness.jpg");
}

VkFormat CastGliToVulkanFormat(gli::format gliFormat) {
    switch (gliFormat)
    {
    case gli::FORMAT_R_ATI1N_UNORM_BLOCK8:
        return VK_FORMAT_BC4_UNORM_BLOCK;
    case gli::FORMAT_RGB_BP_SFLOAT_BLOCK16:
        return VK_FORMAT_BC6H_SFLOAT_BLOCK;
    case  gli::FORMAT_RGBA_BP_UNORM_BLOCK16:
        return VK_FORMAT_BC7_UNORM_BLOCK;
    default:
        ERROR_MSG("Unsupported format!");
    }
    return VK_FORMAT_UNDEFINED;
}

std::string TEXTURE_MANAGER::GetTextureCachedName(const std::string& texName)
{
    std::string cachedName = texName;
    std::replace(cachedName.begin(), cachedName.end(), ' ', '_');
    std::replace(cachedName.begin(), cachedName.end(), '/', '_');
    std::replace(cachedName.begin(), cachedName.end(), '\\', '_');
    std::replace(cachedName.begin(), cachedName.end(), '.', '_');
    return cachedName + ".dds";
}

bool TEXTURE_MANAGER::CreateCachedTexture(const std::string& texSourcePath, const std::string& texDestPath, std::string format)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    const std::wstring converterPath = L"C:/Program Files/NVIDIA Corporation/NVIDIA Texture Tools Exporter/nvtt_export.exe";
    const std::string commandLineSting = texSourcePath + " --format " + format + " --mip-filter kaiser" + " --output " + texDestPath;
    const std::wstring commandLineWstring(commandLineSting.begin(), commandLineSting.end());

    // start the program up
    CreateProcess(converterPath.c_str(),                       // the path
        const_cast<LPWSTR>((L"\"" + converterPath + L"\" " + commandLineWstring).c_str()),        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
    );

    WaitForSingleObject(pi.hProcess, INFINITE);
    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

bool TEXTURE_MANAGER::LoadTexture(const std::string& texName)
{
    auto texIter = m_textureMap.find(texName);
    if (texIter != m_textureMap.end()) {
        return true;
    }

    const std::string cachedTexturePath =  CACHE_TEXTURE_DIR + GetTextureCachedName(texName);

    gli::texture gliTexture = gli::load(cachedTexturePath);
    gli::texture agliTexture = gli::load(TEXTURE_DIR + texName);

    if (gliTexture.empty()) {
        DEBUG_MSG(formatString("Can't load texture %s from cache\n", texName.c_str()).c_str());
        const std::string texturePath = TEXTURE_DIR + texName;
        std::string format = "bc7";
        if (texName.find("Displacement") || texName.find("Roughness") || texName.find("Metalness")) {
            format = "bc4";
        }
        CreateCachedTexture(texturePath, cachedTexturePath, format);
        gliTexture = gli::load(cachedTexturePath);
        if (gliTexture.empty()) {
            ERROR_MSG("Can't create cached version!");
            return false;
        }

    }

    VULKAN_TEXTURE createdTexture;
    VULKAN_TEXTURE_CREATE_DATA createData;
    createData.pData = gliTexture.data<uint8_t>();
    createData.dataSize = gliTexture.size();
    createData.extent.width = static_cast<uint32_t>(gliTexture.mip_level_width(0));
    createData.extent.height = static_cast<uint32_t>(gliTexture.mip_level_height(0));
    createData.extent.depth = static_cast<uint32_t>(gliTexture.mip_level_depth(0));
    createData.format = CastGliToVulkanFormat(gliTexture.format());
    createData.mipLevels = static_cast<uint32_t>(gliTexture.levels());
    createData.usage = VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    size_t mipOffset = 0;
    for (int i = 0; i < gliTexture.levels(); i++) {
        createData.mipLevelsOffsets.push_back(mipOffset);
        mipOffset += gliTexture.size(i);
    }
    VkResult textureCreated = pDrvInterface->CreateTexture(createData, createdTexture);

    gliTexture.clear();
    if (textureCreated != VK_SUCCESS) {
        WARNING_MSG(formatString("%s texture creatino failed!", texName.c_str()).c_str());
        pDrvInterface->DestroyTexture(createdTexture);
        return false;
    }
    m_textureMap.emplace(texName, createdTexture);
    return true;
}

const VULKAN_TEXTURE* TEXTURE_MANAGER::GetTexture (const std::string& texName)
{
    auto texIter = m_textureMap.find(texName);
    if (texIter == m_textureMap.end()) {
        bool isTexLoaded = LoadTexture(texName);
        if (!isTexLoaded) {
            return nullptr;
        }
        texIter = m_textureMap.find(texName);
    }
    return &texIter->second;
}

void TEXTURE_MANAGER::CreateDefalutTextures()
{
    glm::u8vec4 color;

    VULKAN_TEXTURE_CREATE_DATA createData;
    createData.dataSize = sizeof(glm::u8vec4);
    createData.extent.height = 1;
    createData.extent.width = 1;
    createData.extent.depth = 1;
    createData.format = VK_FORMAT_R8G8B8A8_UNORM;
    createData.mipLevels = 1;
    createData.usage = VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    {
        color = glm::u8vec4(0, 0, 0, 255);
        createData.pData = reinterpret_cast<uint8_t*>(&color);
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextures[DEFAULT_BLACK_TEXTURE]);
    }
    {
        color = glm::u8vec4(255, 0, 0, 255);
        createData.pData = reinterpret_cast<uint8_t*>(&color);
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextures[DEFAULT_RED_TEXTURE]);
    }
    {
        color = glm::u8vec4(0, 255, 0, 255);
        createData.pData = reinterpret_cast<uint8_t*>(&color);
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextures[DEFAULT_GREEN_TEXTURE]);
    }
    {
        color = glm::u8vec4(0, 0, 255, 255);
        createData.pData = reinterpret_cast<uint8_t*>(&color);
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextures[DEFAULT_BLUE_TEXTURE]);
    }
    {
        color = glm::u8vec4(255, 255, 255, 255);
        createData.pData = reinterpret_cast<uint8_t*>(&color);
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextures[DEFAULT_WHITE_TEXTURE]);
    }
    {
        color = glm::u8vec4(127, 127, 255, 255);
        createData.pData = reinterpret_cast<uint8_t*>(&color);
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextures[DEFAULT_NORMAL_TEXTURE]);
    }
    {
        color = glm::u8vec4(127, 127, 0, 255);
        createData.pData = reinterpret_cast<uint8_t*>(&color);
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextures[DEFAULT_INVERTED_NORMAL_TEXTURE]);
    }
}

const VULKAN_TEXTURE* TEXTURE_MANAGER::GetDefaultTexture(DEFAULT_TEXTURES defaultTextureId)
{
    ASSERT(defaultTextureId < DEFAULT_LAST_TEXTURE);
    return &m_defaultTextures[defaultTextureId];
}

void TEXTURE_MANAGER::Term()
{
    for (auto texture: m_textureMap) {
        pDrvInterface->DestroyTexture(texture.second);
    }
    for (auto defaultTexture : m_defaultTextures) {
        pDrvInterface->DestroyTexture(defaultTexture);
    }
}
