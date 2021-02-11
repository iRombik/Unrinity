#include "resourceSystem.h"
#include "shaderManager.h"
#include "geometry.h"

#include "ecsCoordinator.h"

#include <glm/gtc/type_ptr.hpp>
#include <gli.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "tiny_gltf.h"

#include "Components/rendered.h"

std::unique_ptr<RESOURCE_SYSTEM> pResourceSystem;

void RESOURCE_SYSTEM::Init()
{
    pShaderManager.reset(new SHADER_MANAGER);
    //pMeshManager.reset(new MESH_MANAGER);
    //pTextureManager.reset(new TEXTURE_MANAGER);
    pVertexDeclarationManager.reset(new VERTEX_DECLARATION_MANAGER);

    pShaderManager->Init();
    //pMeshManager->Init();
    //pTextureManager->Init();
    pVertexDeclarationManager->Init();
}

VkFormat CastGltfToVulkanTextureFormat(int componentNum, int componentType) 
{
    if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        if (componentNum == 4) {
            return VK_FORMAT_R8G8B8A8_SRGB;
        }
    }
    ASSERT_MSG(false, "Unknown texture type!");
    return VK_FORMAT_UNDEFINED;
}

void RESOURCE_SYSTEM::CreateDefaultResources()
{
    CreateDefalutTextures();
}

bool RESOURCE_SYSTEM::LoadModel (const std::string& modelName)
{
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF gltfContext;
    std::string error;
    std::string warning;

    const std::string GLTF_MESH_DIR = "../Media/Meshes/gltf/";
    const std::string GLTF_MESH_EXT = ".gltf";
    std::string levelName = GLTF_MESH_DIR + modelName + "/glTF/" + modelName + GLTF_MESH_EXT;

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

    std::string baseDir = tinygltf::GetBaseDir(levelName);
    m_textureList.resize(gltfModel.images.size());
    for (size_t texId = 0; texId < gltfModel.images.size(); texId++) {
        const tinygltf::Image& image = gltfModel.images[texId];
        bool isTextureLoaded = LoadTexture(image.uri, baseDir, m_textureList[texId]);
        ASSERT(isTextureLoaded);
    }

    m_materialsList.resize(gltfModel.materials.size() + 1);
    for (size_t materialId = 0; materialId < gltfModel.materials.size(); materialId++) {
        const tinygltf::Material& gltfMaterial = gltfModel.materials[materialId];

        MATERIAL_COMPONENT& material = m_materialsList[materialId];

        material.isDoubleSided = gltfMaterial.doubleSided;
        material.baseColor = glm::make_vec4(gltfMaterial.pbrMetallicRoughness.baseColorFactor.data());

        if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index != -1) {
            material.pAlbedoTex = &m_textureList[gltfMaterial.pbrMetallicRoughness.baseColorTexture.index];
        } else {
            material.pAlbedoTex = &m_defaultTextureList[DEFAULT_RED_TEXTURE];
        }

        if (gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index != -1) {
            material.pMetalRoughnessTex = &m_textureList[gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index];
        } else {
            material.pMetalRoughnessTex = &m_defaultTextureList[DEFAULT_GREEN_TEXTURE];
        }

        if (gltfMaterial.normalTexture.index != -1) {
            material.pNormalTex = &m_textureList[gltfMaterial.normalTexture.index];
        } else {
            material.pNormalTex = &m_defaultTextureList[DEFAULT_NORMAL_TEXTURE];
        }
        if (gltfMaterial.emissiveTexture.index != -1) {
            material.pEmissiveTex = &m_textureList[gltfMaterial.emissiveTexture.index];
        } else {
            material.pEmissiveTex = &m_defaultTextureList[DEFAULT_BLACK_TEXTURE];
        }
//         if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
//             material.occlusionTexture = &textures[mat.additionalValues["occlusionTexture"].TextureIndex()];
//             material.texCoordSets.occlusion = mat.additionalValues["occlusionTexture"].TextureTexCoord();
//         }
        material.alphaCutFactor = static_cast<float>(gltfMaterial.alphaCutoff);
        if (gltfMaterial.alphaMode == "BLEND") {
            material.alphaMode = MATERIAL_COMPONENT::ALPHA_MODE::ALPHA_BLEND;
        }
        if (gltfMaterial.alphaMode == "MASK") {
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
    }


    //load meshes
    size_t nufOfPrimitiveMeshes = 0;
    for (size_t meshId = 0; meshId < gltfModel.meshes.size(); meshId++) {
        nufOfPrimitiveMeshes += gltfModel.meshes[meshId].primitives.size();
    }
    m_meshList.resize(nufOfPrimitiveMeshes);
    m_meshHolder.resize(gltfModel.meshes.size());
    size_t primitiveMeshId = 0;
    for (size_t meshId = 0; meshId < gltfModel.meshes.size(); meshId++) {
        const tinygltf::Mesh& gltfMesh = gltfModel.meshes[meshId];
        m_meshHolder[meshId].meshPrimitives.resize(gltfMesh.primitives.size());
        for (size_t primitiveId = 0; primitiveId < gltfMesh.primitives.size(); primitiveId++) {
            const tinygltf::Primitive& gltfPrimitive = gltfMesh.primitives[primitiveId];

            const unsigned char* bufferPos = nullptr;
            size_t posByteStride = 0;
            const unsigned char* bufferNormals = nullptr;
            size_t normByteStride = 0;
            const unsigned char* bufferTexCoordSet0 = nullptr;
            size_t uv0ByteStride = 0;
            const unsigned char* bufferTexCoordSet1 = nullptr;
            size_t uv1ByteStride = 0;
            size_t indexCount = 0;
            bool isMeshHasIndexes = gltfPrimitive.indices > 0;

            const tinygltf::Accessor& posAccessor = gltfModel.accessors[gltfPrimitive.attributes.find("POSITION")->second];
            const tinygltf::BufferView& posView = gltfModel.bufferViews[posAccessor.bufferView];
            bufferPos = &(gltfModel.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]);
            posByteStride = posAccessor.ByteStride(posView);


            if (gltfPrimitive.attributes.find("NORMAL") != gltfPrimitive.attributes.end()) {
                const tinygltf::Accessor& normAccessor = gltfModel.accessors[gltfPrimitive.attributes.find("NORMAL")->second];
                const tinygltf::BufferView& normView = gltfModel.bufferViews[normAccessor.bufferView];
                bufferNormals = &(gltfModel.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]);
                normByteStride = normAccessor.ByteStride(normView);
            }

            if (gltfPrimitive.attributes.find("TEXCOORD_0") != gltfPrimitive.attributes.end()) {
                const tinygltf::Accessor& uvAccessor = gltfModel.accessors[gltfPrimitive.attributes.find("TEXCOORD_0")->second];
                const tinygltf::BufferView& uvView = gltfModel.bufferViews[uvAccessor.bufferView];
                bufferTexCoordSet0 = &(gltfModel.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]);
                uv0ByteStride = uvAccessor.ByteStride(uvView);
            }

            if (gltfPrimitive.attributes.find("TEXCOORD_1") != gltfPrimitive.attributes.end()) {
                const tinygltf::Accessor& uvAccessor = gltfModel.accessors[gltfPrimitive.attributes.find("TEXCOORD_1")->second];
                const tinygltf::BufferView& uvView = gltfModel.bufferViews[uvAccessor.bufferView];
                bufferTexCoordSet1 = &(gltfModel.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]);
                uv1ByteStride = uvAccessor.ByteStride(uvView);
            }

            //TODO: use separated buffers, don't blend them into one buffer
            std::vector<SIMPLE_VERTEX> vertexBuf(posAccessor.count);
            for (int verId = 0; verId < posAccessor.count; verId++) {
                SIMPLE_VERTEX& vertex = vertexBuf[verId];
                vertex.position = glm::make_vec3((float*)&bufferPos[verId * posByteStride]) / 10.f;
                if (bufferTexCoordSet0) {
                    vertex.texCoord = glm::make_vec2((float*)&bufferTexCoordSet0[verId * uv0ByteStride]);
                } else {
                    vertex.texCoord = glm::vec2(0.f, 0.f);
                }
                if (bufferNormals) {
                    vertex.normal = glm::make_vec3((float*)&bufferNormals[verId * normByteStride]);
                } else {
                    vertex.normal = glm::vec3(0.f, 1.f, 0.f);
                }
            }

            std::vector<uint16_t> indexBuf;
            if (isMeshHasIndexes) {
                const tinygltf::Accessor& accessor = gltfModel.accessors[gltfPrimitive.indices];
                const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
                
                indexCount = accessor.count;
                const void* pData = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

                switch (accessor.componentType) {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: 
                    {
                        const uint32_t* pData32 = static_cast<const uint32_t*>(pData);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuf.push_back(pData32[index]);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                    {
                        const uint16_t* pData16 = static_cast<const uint16_t*>(pData);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuf.push_back(pData16[index]);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: 
                    {
                        const uint8_t* pData8 = static_cast<const uint8_t*>(pData);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuf.push_back(pData8[index]);
                        }
                        break;
                    }
                    default:
                    {
                        ERROR_MSG("Not implemented type");
                        return false;
                    }
                }
            }

            VULKAN_MESH mesh;
            mesh.vertexFormatId = SIMPLE_VERTEX::formatId;
            mesh.numOfIndexes = indexBuf.size();
            mesh.numOfVertexes = vertexBuf.size();

            VkResult vertexBufferCreated = VK_SUCCESS;
            VkBufferCreateInfo vertexBufferInfo = {};
            vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vertexBufferInfo.size = vertexBuf.size() * sizeof(SIMPLE_VERTEX);
            vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            //alloc memory and init buffer
            vertexBufferCreated =
                pDrvInterface->CreateAndFillBuffer(vertexBufferInfo, (const uint8_t*)vertexBuf.data(), false, mesh.vertexBuffer);

            VkResult indexBufferCreated = VK_SUCCESS;
            if (isMeshHasIndexes) {
                VkBufferCreateInfo indexBufferInfo = {};
                indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                indexBufferInfo.size = indexBuf.size() * sizeof(uint16_t);
                indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                indexBufferCreated =
                    pDrvInterface->CreateAndFillBuffer(indexBufferInfo, (const byte*)indexBuf.data(), false, mesh.indexBuffer);
            }

            if (vertexBufferCreated != VK_SUCCESS || indexBufferCreated != VK_SUCCESS) {
                ASSERT_MSG(vertexBufferCreated == VK_SUCCESS, "Vertex buffer not created!");
                ASSERT_MSG(indexBufferCreated == VK_SUCCESS, "Index buffer not created!");
                //DestroyMesh(primitiveMesh);
                return false;
            }
            m_meshList[primitiveMeshId] = mesh;

            MESH_PRIMITIVE& primitiveMesh = m_meshHolder[meshId].meshPrimitives[primitiveId];
            primitiveMesh.aabb.minPos = glm::make_vec3(posAccessor.minValues.data());
            primitiveMesh.aabb.maxPos = glm::make_vec3(posAccessor.maxValues.data());
            primitiveMesh.pMaterial = &m_materialsList[gltfPrimitive.material];
            primitiveMesh.pMesh = &m_meshList[primitiveMeshId];
            primitiveMesh.pParentHolder = &m_meshHolder[meshId];

            ECS::ENTITY_TYPE primEntity = ECS::pEcsCoordinator->CreateEntity();
            ECS::pEcsCoordinator->AddComponentToEntity(primEntity, primitiveMesh);
            ECS::pEcsCoordinator->AddComponentToEntity(primEntity, RENDERED_COMPONENT());

            m_meshHolder[meshId].aabb.minPos = glm::min(m_meshHolder[meshId].aabb.minPos, primitiveMesh.aabb.minPos);
            m_meshHolder[meshId].aabb.maxPos = glm::min(m_meshHolder[meshId].aabb.maxPos, primitiveMesh.aabb.maxPos);
            primitiveMeshId++;
        }
    }

    const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
    m_nodeList.resize(scene.nodes.size());
    for (size_t nodeId = 0; nodeId < scene.nodes.size(); nodeId++) {
        const tinygltf::Node& gltfNode = gltfModel.nodes[scene.nodes[nodeId]];

        NODE_COMPONENT& node = m_nodeList[nodeId];
        if (gltfNode.translation.size() == 3) {
            node.translation = glm::make_vec3(gltfNode.translation.data());
        } 
        if (gltfNode.rotation.size() == 4) {
            node.rotation = glm::make_quat(gltfNode.rotation.data());
        }
        if (gltfNode.scale.size() == 3) {
            node.scale = glm::make_vec3(gltfNode.scale.data());
        }
        if (gltfNode.matrix.size() == 16) {
            node.matrix = glm::make_mat4x4(gltfNode.matrix.data());
        }
        if (gltfNode.mesh > -1) {
            m_meshHolder[gltfNode.mesh].pParentsNodes.push_back(&m_nodeList[nodeId]);
            node.mesh = &m_meshHolder[gltfNode.mesh];
        }
    }

    return true;
}

void RESOURCE_SYSTEM::UnloadScene()
{
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

std::string GetTextureCachedName(const std::string& texName)
{
    std::string cachedName = texName;
    std::replace(cachedName.begin(), cachedName.end(), ' ', '_');
    std::replace(cachedName.begin(), cachedName.end(), '/', '_');
    std::replace(cachedName.begin(), cachedName.end(), '\\', '_');
    std::replace(cachedName.begin(), cachedName.end(), '.', '_');
    return cachedName + ".dds";
}

bool CreateCachedTexture(const std::string& texSourcePath, const std::string& texDestPath, std::string format)
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

bool RESOURCE_SYSTEM::LoadTexture(const std::string& textureName, const std::string& textureDir, VULKAN_TEXTURE& texture)
{
    const std::string cachedTexturePath = CACHE_TEXTURE_DIR + GetTextureCachedName(textureName);

    gli::texture gliTexture = gli::load(cachedTexturePath);

    if (gliTexture.empty()) {
        DEBUG_MSG(formatString("Can't load texture %s from cache\n", textureName.c_str()).c_str());
        const std::string texturePath = textureDir + "/" + textureName;
        std::string format = "bc7";
//         if (textureName.find("Displacement") || textureName.find("Roughness") || textureName.find("Metalness")) {
//             format = "bc4";
//         }
        CreateCachedTexture(texturePath, cachedTexturePath, format);
        gliTexture = gli::load(cachedTexturePath);
        if (gliTexture.empty()) {
            ERROR_MSG("Can't create cached version!");
            return false;
        }

    }

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
    VkResult textureCreated = pDrvInterface->CreateTexture(createData, texture);

    gliTexture.clear();
    if (textureCreated != VK_SUCCESS) {
        WARNING_MSG(formatString("%s texture creatino failed!", textureName.c_str()).c_str());
        pDrvInterface->DestroyTexture(texture);
        return false;
    }
    return true;
}

void RESOURCE_SYSTEM::CreateDefalutTextures()
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
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextureList[DEFAULT_BLACK_TEXTURE]);
    }
    {
        color = glm::u8vec4(255, 0, 0, 255);
        createData.pData = reinterpret_cast<uint8_t*>(&color);
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextureList[DEFAULT_RED_TEXTURE]);
    }
    {
        color = glm::u8vec4(0, 255, 0, 255);
        createData.pData = reinterpret_cast<uint8_t*>(&color);
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextureList[DEFAULT_GREEN_TEXTURE]);
    }
    {
        color = glm::u8vec4(0, 0, 255, 255);
        createData.pData = reinterpret_cast<uint8_t*>(&color);
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextureList[DEFAULT_BLUE_TEXTURE]);
    }
    {
        color = glm::u8vec4(255, 255, 255, 255);
        createData.pData = reinterpret_cast<uint8_t*>(&color);
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextureList[DEFAULT_WHITE_TEXTURE]);
    }
    {
        color = glm::u8vec4(127, 127, 255, 255);
        createData.pData = reinterpret_cast<uint8_t*>(&color);
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextureList[DEFAULT_NORMAL_TEXTURE]);
    }
    {
        color = glm::u8vec4(127, 127, 0, 255);
        createData.pData = reinterpret_cast<uint8_t*>(&color);
        VkResult result = pDrvInterface->CreateTexture(createData, m_defaultTextureList[DEFAULT_INVERTED_NORMAL_TEXTURE]);
    }
}

void RESOURCE_SYSTEM::Reload()
{
    pShaderManager->ReloadShaders();
}

void RESOURCE_SYSTEM::Term()
{
    pShaderManager->TermShaders();
    pShaderManager.release();
}

bool RESOURCE_SYSTEM::LoadShaders()
{
    pShaderManager->LoadShaders();
    return true;
}

void RESOURCE_SYSTEM::ReloadShaders()
{
    pDrvInterface->WaitGPU();
    pDrvInterface->DropPiplineStateCache();
    pShaderManager->ReloadShaders();
}
