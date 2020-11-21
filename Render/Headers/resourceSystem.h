#pragma once
#include <string>
#include "vulkanResourcesDescription.h"
#include "materialManager.h"
#include "meshManager.h"

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

    bool LoadModel(const std::string& levelName);
    void UnloadScene();

    VULKAN_MESH*    LoadMesh(const std::string& meshName);
    VULKAN_TEXTURE* LoadTexture(const std::string& textureName);
private:
    std::vector<VULKAN_MESH>        m_meshList;
    std::vector<MESH_HOLDER_COMPONENT>        m_meshHolder;
    std::vector<VULKAN_TEXTURE>     m_textureList;
    std::vector<MATERIAL_COMPONENT> m_materialsList;
    std::vector<NODE_COMPONENT>     m_nodeList;
};

extern std::unique_ptr<RESOURCE_SYSTEM> pResourceSystem;
