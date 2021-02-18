#pragma once
#include <string>
#include "vulkanResourcesDescription.h"
#include "materialManager.h"
#include "meshManager.h"

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

struct MESH_HOLDER_COMPONENT;
struct NODE_COMPONENT;

struct AABB
{
    glm::vec2 minPos;
    glm::vec2 maxPos;
};

struct MESH_PRIMITIVE : public ECS::COMPONENT<MESH_PRIMITIVE>
{
    const VULKAN_MESH*           pMesh;
    const MATERIAL_COMPONENT*    pMaterial;
    const MESH_HOLDER_COMPONENT* pParentHolder;
    AABB aabb;
};

struct MESH_HOLDER_COMPONENT 
{
    std::vector<MESH_PRIMITIVE>  meshPrimitives;
    std::vector<NODE_COMPONENT*> pParentsNodes;
    AABB aabb;
};

struct NODE_COMPONENT//: public ECS::COMPONENT<NODE_COMPONENT>
{
    glm::mat4x4 matrix;
    MESH_HOLDER_COMPONENT* mesh;

    NODE_COMPONENT* pParent;
    std::vector<NODE_COMPONENT*> pChildren;

    glm::vec3 translation;
    glm::vec3 scale;
    glm::quat rotation;
};

struct MODEL_COMPONENT// : public ECS::COMPONENT<MODEL_COMPONENT>
{
    std::vector<NODE_COMPONENT> nodes;
};

class RESOURCE_SYSTEM {
public:
    void Init();
    void Reload();
    void Term();

    bool LoadShaders();
    void ReloadShaders();

    void CreateDefaultResources();
    bool LoadModel(const std::string& levelName);
    void UnloadScene();

    const VULKAN_TEXTURE* GetDefaultTexture(int textureId) const { return &m_defaultTextureList[textureId]; }
private:
    bool LoadMesh(const std::string& meshName);
    bool LoadTexture(const std::string& textureName, const std::string& textureDir, VULKAN_TEXTURE& texture);
    void CreateDefalutTextures();
private:
    const std::string CACHE_TEXTURE_DIR = "../Media/Textures/_textureCache/";
    static const uint32_t MAX_ARRAY_SIZE = 1024;

    std::array<VULKAN_MESH, MAX_ARRAY_SIZE>           m_meshList;
    std::array<MESH_HOLDER_COMPONENT, MAX_ARRAY_SIZE> m_meshHolderList;
    std::array<VULKAN_TEXTURE, MAX_ARRAY_SIZE>        m_textureList;
    std::array<MATERIAL_COMPONENT, MAX_ARRAY_SIZE>    m_materialsList;
    std::array<NODE_COMPONENT, MAX_ARRAY_SIZE>        m_nodeList;

    uint32_t m_meshNum;
    uint32_t m_meshHolderNum;
    uint32_t m_textureNum;
    uint32_t m_materialNum;
    uint32_t m_nodeNum;

    std::array<VULKAN_TEXTURE, DEFAULT_LAST_TEXTURE> m_defaultTextureList;
    MATERIAL_COMPONENT m_defaultMaterial;
    MESH_HOLDER_COMPONENT m_defaultMesh;
};

extern std::unique_ptr<RESOURCE_SYSTEM> pResourceSystem;
