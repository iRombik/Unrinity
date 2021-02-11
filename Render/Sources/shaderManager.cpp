#include "shaderManager.h"
#include "support.h"
#include "vulkanDriver.h"

#include <dxc/dxcapi.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

std::unique_ptr<SHADER_MANAGER> pShaderManager;

namespace EFFECT_DATA {
    std::string SHADER_NAMES[] = {
        "fullscreen",
        "fillGBuffer",
        "shadeGBuffer",
        "shadow",
        "terrain",
        "ui"
    };

    std::unordered_map<EFFECT_DATA::SHADER_TYPE, std::string> SHADER_TYPE_TO_NAME_CAST = {
    { EFFECT_DATA::SHADER_TYPE::VERTEX, "vs"},
    { EFFECT_DATA::SHADER_TYPE::PIXEL,  "ps" },
    };
    std::unordered_map<std::string, EFFECT_DATA::SHADER_TYPE> SHADER_NAME_TO_TYPE_CAST = {
        { "vs", EFFECT_DATA::SHADER_TYPE::VERTEX },
        { "ps", EFFECT_DATA::SHADER_TYPE::PIXEL },
    };
}

void SHADER_MANAGER::Init()
{
    InitShaderDecriptorLayoutTable();
}

VkDescriptorSetLayoutBinding CreateLayoutBinding(uint32_t bindingSlot, VkDescriptorType descriptorType, VkShaderStageFlagBits stageBitFlags, EFFECT_DATA::SAMPLERS samplerId = EFFECT_DATA::SAMPLERS::SAMPLER_LAST)
{
    VkDescriptorSetLayoutBinding layout;
    layout.binding = bindingSlot;
    layout.descriptorType = descriptorType;
    layout.stageFlags = stageBitFlags;
    layout.descriptorCount = 1;
    layout.pImmutableSamplers = nullptr;
    //work only with VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    //layout.pImmutableSamplers = &m_samplers[samplerId];
    return layout;
}

void SHADER_MANAGER::InitShaderDecriptorLayoutTable()
{
    const size_t shrFullscreenId = EFFECT_DATA::SHR_FULLSCREEN;
    m_shaderDesc[shrFullscreenId].push_back(CreateLayoutBinding(20, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT));

    const size_t shrFillGBufferId = EFFECT_DATA::SHR_FILL_GBUFFER;
    m_shaderDesc[shrFillGBufferId].push_back(CreateLayoutBinding(EFFECT_DATA::CONST_BUFFERS_SLOT[EFFECT_DATA::CB_COMMON_DATA], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS));
    m_shaderDesc[shrFillGBufferId].push_back(CreateLayoutBinding(EFFECT_DATA::CONST_BUFFERS_SLOT[EFFECT_DATA::CB_MATERIAL], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderDesc[shrFillGBufferId].push_back(CreateLayoutBinding(20, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderDesc[shrFillGBufferId].push_back(CreateLayoutBinding(21, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderDesc[shrFillGBufferId].push_back(CreateLayoutBinding(22, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT));

    const size_t shrShadeGBufferId = EFFECT_DATA::SHR_SHADE_GBUFFER;
    m_shaderDesc[shrShadeGBufferId].push_back(CreateLayoutBinding(EFFECT_DATA::CONST_BUFFERS_SLOT[EFFECT_DATA::CB_COMMON_DATA], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderDesc[shrShadeGBufferId].push_back(CreateLayoutBinding(EFFECT_DATA::CONST_BUFFERS_SLOT[EFFECT_DATA::CB_LIGHTS], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderDesc[shrShadeGBufferId].push_back(CreateLayoutBinding(20, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderDesc[shrShadeGBufferId].push_back(CreateLayoutBinding(21, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderDesc[shrShadeGBufferId].push_back(CreateLayoutBinding(22, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderDesc[shrShadeGBufferId].push_back(CreateLayoutBinding(23, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderDesc[shrShadeGBufferId].push_back(CreateLayoutBinding(24, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderDesc[shrShadeGBufferId].push_back(CreateLayoutBinding(25, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT));

    const size_t shrShadow = EFFECT_DATA::SHR_SHADOW;
    m_shaderDesc[shrShadow].push_back(CreateLayoutBinding(EFFECT_DATA::CONST_BUFFERS_SLOT[EFFECT_DATA::CB_LIGHTS], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT));

    const size_t shrTerrain = EFFECT_DATA::SHR_TERRAIN;
    m_shaderDesc[shrTerrain].push_back(CreateLayoutBinding(EFFECT_DATA::CONST_BUFFERS_SLOT[EFFECT_DATA::CB_COMMON_DATA], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT));
    m_shaderDesc[shrTerrain].push_back(CreateLayoutBinding(EFFECT_DATA::CONST_BUFFERS_SLOT[EFFECT_DATA::CB_LIGHTS], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderDesc[shrTerrain].push_back(CreateLayoutBinding(EFFECT_DATA::CONST_BUFFERS_SLOT[EFFECT_DATA::CB_TERRAIN], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT));

    const size_t shrUiId = EFFECT_DATA::SHR_UI;
    m_shaderDesc[shrUiId].push_back(CreateLayoutBinding(EFFECT_DATA::CONST_BUFFERS_SLOT[EFFECT_DATA::CB_UI], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT));
    m_shaderDesc[shrUiId].push_back(CreateLayoutBinding(20, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderDesc[shrUiId].push_back(CreateLayoutBinding(21, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT));

    for (int i = 0; i < EFFECT_DATA::SHR_LAST; i++) {
        for (int s = 0; s < NUM_SAMPLERS; s++) {
            m_shaderDesc[i].push_back(CreateLayoutBinding(120 + s, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS));
        }
    }
}

void SHADER_MANAGER::CompileShader(uint8_t passId, EFFECT_DATA::SHADER_TYPE type) const
{
    std::vector<std::string> defines;
    //todo
    const std::wstring absolutePath = L"C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders\\";
    LPWSTR lpFilename = nullptr;
    DWORD nSize = 0;
    const std::wstring shaderName = std::wstring(EFFECT_DATA::SHADER_NAMES[passId].begin(), EFFECT_DATA::SHADER_NAMES[passId].end());
    const std::wstring typeName = std::wstring(EFFECT_DATA::SHADER_TYPE_TO_NAME_CAST[type].begin(), EFFECT_DATA::SHADER_TYPE_TO_NAME_CAST[type].end());

    const std::wstring converterPath = L"C:\\Users\\Admin\\Desktop\\Unrinity\\Libs\\DXC\\bin\\dxc.exe";
    std::vector<std::wstring> commandLineParams;
    //compile spir-v
    commandLineParams.push_back(L" -spirv");
    commandLineParams.push_back(L"-Zi");
// #ifdef OUT_DEBUG_INFO
//     //for render doc pixel debugging
//     commandLineParams.push_back(L"-fspv-extension=SPV_GOOGLE_user_type");
// #endif
    //file
    const std::wstring path = absolutePath + shaderName + typeName + L".fx";
    commandLineParams.push_back(std::wstring(path.begin(), path.end()));
//     //error file
    const std::wstring errorPath = absolutePath + L"compilationErrors\\" + shaderName + typeName + L"error.txt";
    commandLineParams.push_back(L"-Fe");
    commandLineParams.push_back(errorPath);
    //output file
    const std::wstring outPath = absolutePath + L"binaries\\" + shaderName + L"." + typeName;
    commandLineParams.push_back(L"-Fo");
    commandLineParams.push_back(outPath);
    //enter function
    commandLineParams.push_back(L"-E");
    commandLineParams.push_back(L"main");
    //target
    commandLineParams.push_back(L"-T");
    commandLineParams.push_back(typeName + L"_6_6");
    //defines
    for (const auto& define : defines) {
        commandLineParams.push_back(L"-D");
        commandLineParams.push_back(std::wstring(define.begin(), define.end())); //uuuh!
    }
    
    std::wstring commandLineString;
    for (const auto& command : commandLineParams) {
        commandLineString += command + L" ";
    }
    commandLineString += L"\0";

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // start the program up
    CreateProcess(
        converterPath.data(),                       // the path
        commandLineString.data(),        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        CREATE_NO_WINDOW,  // creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
    );

    WaitForSingleObject(pi.hProcess, INFINITE);
    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

size_t SHADER_MANAGER::GetShaderId(const std::string& shaderName) const
{
    //todo: map
    for (size_t shaderId = 0; shaderId < EFFECT_DATA::SHR_LAST; shaderId++) {
        const std::string& name = EFFECT_DATA::SHADER_NAMES[shaderId];
        if (name == shaderName) {
            return shaderId;
        }
    }
    return EFFECT_DATA::SHR_LAST;
}


void SHADER_MANAGER::LoadShaders()
{
    const bool isDXC = true;
    for (int passId = 0; passId < EFFECT_DATA::SHADER_ID::SHR_LAST; passId++) {
        for (int typeId = 0; typeId < (int)EFFECT_DATA::SHADER_TYPE::LAST; typeId++) {
            EFFECT_DATA::SHADER_TYPE shaderType = (EFFECT_DATA::SHADER_TYPE)typeId;
            const std::string& shaderName = EFFECT_DATA::SHADER_NAMES[passId];
            const std::string& typeName = EFFECT_DATA::SHADER_TYPE_TO_NAME_CAST[shaderType];
            std::string filePath = SHADERS_FOLDER;
            if (isDXC) {
                CompileShader(passId, shaderType);
                filePath += shaderName + "." + typeName;
            } else {
                ASSERT(false);
            }
            std::vector<char> shaderRawData = std::move(ReadFile(filePath));
            SHADER_MODULE createdShader;
            bool isShaderCreated = pDrvInterface->CreateShader(shaderRawData, createdShader.shader);
            if (!isShaderCreated) {
                WARNING_MSG(formatString("Failed to create shader module %s. Shader type: %s!", shaderName.c_str(), typeName.c_str()).c_str());
                continue;
            }
            createdShader.passId = passId;
            createdShader.typeId = typeId;
            if (shaderType == EFFECT_DATA::SHADER_TYPE::VERTEX) {
                m_vertexShaderModules[passId] = createdShader;
            }
            if (shaderType == EFFECT_DATA::SHADER_TYPE::PIXEL) {
                m_pixelShaderModules[passId] = createdShader;
            }
        }
    }
}

void SHADER_MANAGER::ReloadShaders()
{
    TermShaders();
    LoadShaders();
}

void SHADER_MANAGER::TermShaders()
{
    for (SHADER_MODULE& shaderModule : m_vertexShaderModules) {
        pDrvInterface->DestroyShader(shaderModule.shader);
    }
    for (SHADER_MODULE& shaderModule : m_pixelShaderModules) {
        pDrvInterface->DestroyShader(shaderModule.shader);
    }
}

const std::vector<VkDescriptorSetLayoutBinding>& SHADER_MANAGER::GetDecriptorLayouts(uint8_t shaderId) const
{
    return m_shaderDesc[shaderId];
}

const VkShaderModule& SHADER_MANAGER::GetVertexShader(uint8_t shaderId) const
{
    return m_vertexShaderModules[shaderId].shader;
}

const VkShaderModule& SHADER_MANAGER::GetPixelShader(uint8_t shaderId) const
{
    return m_pixelShaderModules[shaderId].shader;
}
