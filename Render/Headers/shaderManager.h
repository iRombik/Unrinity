#pragma once
#include <array>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <memory>

#include "effectData.h"

struct SHADER_MODULE {
    SHADER_MODULE() : passId(-1), typeId(EFFECT_DATA::SHADER_TYPE::LAST), shader(VK_NULL_HANDLE) {}
    int passId;
    int typeId;
    VkShaderModule shader;
};

class SHADER_MANAGER {
public:
    void Init();

    void LoadShaders();
    void ReloadShaders();
    void TermShaders();

    const std::vector<VkDescriptorSetLayoutBinding>& GetDecriptorLayouts(uint8_t shaderId) const;
    const VkShaderModule& GetVertexShader(uint8_t shaderId) const;
    const VkShaderModule& GetPixelShader(uint8_t shaderId) const;
private:
    size_t GetShaderId(const std::string& shaderName) const;
    void   InitShaderDecriptorLayoutTable();
    void   CompileShader(uint8_t shaderId, EFFECT_DATA::SHADER_TYPE type) const;
private:
    const std::string SHADERS_SOURES_FOLDER = "..\\shaders\\";
    const std::string SHADERS_FOLDER = "..\\shaders\\binaries\\";

    std::array<std::vector<VkDescriptorSetLayoutBinding>, EFFECT_DATA::SHR_LAST> m_shaderDesc;
    std::array<SHADER_MODULE, EFFECT_DATA::SHR_LAST> m_vertexShaderModules;
    std::array<SHADER_MODULE, EFFECT_DATA::SHR_LAST> m_pixelShaderModules;
};

extern std::unique_ptr<SHADER_MANAGER> pShaderManager;