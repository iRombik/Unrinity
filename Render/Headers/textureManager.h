#pragma once
#include "geometry.h"

#include <memory>
#include <unordered_map>

enum DEFAULT_TEXTURES {
    DEFAULT_BLACK_TEXTURE,
    DEFAULT_RED_TEXTURE,
    DEFAULT_GREEN_TEXTURE,
    DEFAULT_BLUE_TEXTURE,
    DEFAULT_WHITE_TEXTURE,

    DEFAULT_NORMAL_TEXTURE,
    DEFAULT_INVERTED_NORMAL_TEXTURE,

    DEFAULT_LAST_TEXTURE
};

class TEXTURE_MANAGER {
public:
    void Init();
    void LoadCommonTextures();
    const VULKAN_TEXTURE* GetTexture(const std::string& texName);
    const VULKAN_TEXTURE* GetDefaultTexture(DEFAULT_TEXTURES defaultTextureId);
    void Term();
private:
    std::string GetTextureCachedName(const std::string& texName);
    bool        CreateCachedTexture(const std::string& texSourcePath, const std::string& texDestPath, std::string format);
    bool        LoadTexture(const std::string& texName);

    void        CreateDefalutTextures();

private:
    const std::string TEXTURE_DIR       = "../Media/Textures/";
    const std::string CACHE_TEXTURE_DIR = "../Media/Textures/_textureCache/";

    std::unordered_map<std::string, VULKAN_TEXTURE> m_textureMap;
    std::array<VULKAN_TEXTURE, DEFAULT_LAST_TEXTURE> m_defaultTextures;
};

extern std::unique_ptr<TEXTURE_MANAGER> pTextureManager;
