#include "resourceSystem.h"
#include "shaderManager.h"
#include "meshManager.h"
#include "textureManager.h"
#include "materialManager.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
//#define TINYGLTF_NOEXCEPTION
//#define JSON_NOEXCEPTION
#include "tiny_gltf.h"

std::unique_ptr<RESOURCE_SYSTEM> pResourceSystem;

void RESOURCE_SYSTEM::Init()
{
    pShaderManager.reset(new SHADER_MANAGER);
    pMeshManager.reset(new MESH_MANAGER);
    pTextureManager.reset(new TEXTURE_MANAGER);
    pVertexDeclarationManager.reset(new VERTEX_DECLARATION_MANAGER);

    pShaderManager->Init();
    pMeshManager->Init();
    pTextureManager->Init();
    pVertexDeclarationManager->Init();
}

VkFormat CastGltfToVulkanTextureFormat(int componentNum, int componentType) 
{
    if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        if (componentNum == 4) {
            return VK_FORMAT_R8G8B8A8_UINT;
        }
    }
    ASSERT_MSG(false, "Unknown texture type!");
    return VK_FORMAT_UNDEFINED;
}

bool RESOURCE_SYSTEM::LoadScene(const std::string& levelName1)
{
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF gltfContext;
    std::string error;
    std::string warning;

    const std::string GLTF_MESH_DIR = "../Media/Meshes/gltf/";
    const std::string GLTF_MESH_EXT = ".gltf";
    std::string levelName = GLTF_MESH_DIR + levelName1 + "/glTF/" + levelName1 + GLTF_MESH_EXT;

    bool binary = false;
    size_t extpos = levelName.rfind('.', levelName.length());
    if (extpos != std::string::npos) {
        binary = (levelName.substr(extpos + 1, levelName.length() - extpos) == "glb");
    }

    bool fileLoaded = false;
    if (binary) {
        fileLoaded = gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, levelName.c_str());
    } else {
        fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, levelName.c_str());
    }

    if (!fileLoaded) {
        WARNING_MSG(formatString("Can't open %s mesh file!", levelName.c_str()).c_str());
        return false;
    }

    std::vector<VULKAN_TEXTURE> textures(gltfModel.images.size());
    for (size_t texId = 0; texId < gltfModel.images.size(); texId++) {
        const tinygltf::Image& image = gltfModel.images[texId];

        VULKAN_TEXTURE_CREATE_DATA createData;
        createData.dataSize = image.height * image.width * image.component * (image.bits / 8);
        createData.extent.height = image.height;
        createData.extent.width = image.width;
        createData.extent.depth = 1;
        createData.format = CastGltfToVulkanTextureFormat(image.component, image.pixel_type);
        createData.mipLevels = 1;
        createData.pData = static_cast<const uint8_t*>(image.image.data());
        createData.usage = VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        VkResult result = pDrvInterface->CreateTexture(createData, textures[texId]);
        ASSERT(result == VK_SUCCESS);
    }

    std::vector<MATERIAL_COMPONENT> materials;
    for (size_t materialId = 0; materialId < gltfModel.materials.size(); materialId++) {
        const tinygltf::Material& mat = gltfModel.materials[materialId];

        MATERIAL_COMPONENT material;

        material.isDoubleSided = mat.doubleSided;
        material.baseColor.x = mat.pbrMetallicRoughness.baseColorFactor[0];
        material.baseColor.y = mat.pbrMetallicRoughness.baseColorFactor[1];
        material.baseColor.z = mat.pbrMetallicRoughness.baseColorFactor[2];
        material.baseColor.w = mat.pbrMetallicRoughness.baseColorFactor[3];

        if (mat.pbrMetallicRoughness.baseColorTexture.index != -1) {
            material.pAlbedoTex = &textures[mat.pbrMetallicRoughness.baseColorTexture.index];
        }
        if (mat.pbrMetallicRoughness.metallicRoughnessTexture.index != -1) {
            material.pMetalnessTex = &textures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index];
        }
        if (mat.normalTexture.index != -1) {
            material.pNormalTex = &textures[mat.normalTexture.index];
        }
        if (mat.emissiveTexture.index != -1) {
            material.pEmissiveTex = &textures[mat.emissiveTexture.index];
        }
//         if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
//             material.occlusionTexture = &textures[mat.additionalValues["occlusionTexture"].TextureIndex()];
//             material.texCoordSets.occlusion = mat.additionalValues["occlusionTexture"].TextureTexCoord();
//         }
        material.alphaCutFactor = mat.alphaCutoff;
        if (mat.alphaMode == "BLEND") {
            material.alphaMode = MATERIAL_COMPONENT::ALPHA_MODE::ALPHA_BLEND;
        }
        if (mat.alphaMode == "MASK") {
            material.alphaMode = MATERIAL_COMPONENT::ALPHA_MODE::ALPHA_MASK;
        }
//         if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
//             material.emissiveFactor = glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
//             material.emissiveFactor = glm::vec4(0.0f);
//         }

        // Extensions
        // @TODO: Find out if there is a nicer way of reading these properties with recent tinygltf headers
//         if (mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end()) {
//             auto ext = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
//             if (ext->second.Has("specularGlossinessTexture")) {
//                 auto index = ext->second.Get("specularGlossinessTexture").Get("index");
//                 material.extension.specularGlossinessTexture = &textures[index.Get<int>()];
//                 auto texCoordSet = ext->second.Get("specularGlossinessTexture").Get("texCoord");
//                 material.texCoordSets.specularGlossiness = texCoordSet.Get<int>();
//                 material.pbrWorkflows.specularGlossiness = true;
//             }
//             if (ext->second.Has("diffuseTexture")) {
//                 auto index = ext->second.Get("diffuseTexture").Get("index");
//                 material.extension.diffuseTexture = &textures[index.Get<int>()];
//             }
//             if (ext->second.Has("diffuseFactor")) {
//                 auto factor = ext->second.Get("diffuseFactor");
//                 for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
//                     auto val = factor.Get(i);
//                     material.extension.diffuseFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
//                 }
//             }
//             if (ext->second.Has("specularFactor")) {
//                 auto factor = ext->second.Get("specularFactor");
//                 for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
//                     auto val = factor.Get(i);
//                     material.extension.specularFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
//                 }
//             }
//         }

        materials.push_back(material);
    }
    materials.push_back(MATERIAL_COMPONENT());
    const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
    for (size_t nodeId = 0; nodeId < scene.nodes.size(); nodeId++) {
        const tinygltf::Node& node = gltfModel.nodes[scene.nodes[nodeId]];
    }

    return true;
}

void RESOURCE_SYSTEM::UnloadScene()
{
}

void RESOURCE_SYSTEM::Reload()
{
    pShaderManager->ReloadShaders();
}

void RESOURCE_SYSTEM::Term()
{
    pShaderManager->TermShaders();
    pMeshManager->Term();
    pTextureManager->Term();

    pShaderManager.release();
    pMeshManager.release();
    pTextureManager.release();
}

bool RESOURCE_SYSTEM::LoadShaders()
{
    pShaderManager->LoadShaders();
    return true;
}

void RESOURCE_SYSTEM::ReloadShaders()
{
}
