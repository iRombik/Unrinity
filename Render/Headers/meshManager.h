#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "vulkanDriver.h"
#include "componentManager.h"

class MESH_LOADER;
class GLTF_LOADER;
class MESH_MANAGER;
struct VULKAN_MESH;

struct MESH_COMPONENT : public ECS::COMPONENT<VULKAN_MESH>
{
    const VULKAN_MESH* pMesh;
};

struct NODE_COMPONENT : public ECS::COMPONENT<NODE_COMPONENT>
{
    NODE_COMPONENT* pParent;
    std::vector<NODE_COMPONENT*> pChildren;

    glm::mat4x4 transofmMatrix;
    MESH_COMPONENT* pMeshComponent;
};

struct MODEL_COMPONENT : public ECS::COMPONENT<MODEL_COMPONENT>
{
    std::vector<NODE_COMPONENT> nodes;
};

class MESH_MANAGER {
public:
    MESH_MANAGER();
    ~MESH_MANAGER();

    void Init();
    void LoadCommonMeshes();
    void Term();

    VULKAN_MESH* GetMeshByName(const std::string& meshName);
private:
    const std::string OBJ_MESH_DIR = "../Media/Meshes/obj";
    const std::string OBJ_MESH_EXT = ".obj";

    const std::string GLTF_MESH_DIR = "../Media/Meshes/gltf/";
    const std::string GLTF_MESH_EXT = ".gltf";

    std::string GetOBJMeshPath(const std::string& meshName);
    std::string GetGLTFMeshPath(const std::string& meshName);

    bool  LoadMesh(const std::string& meshName);
    void  DestroyMesh (VULKAN_MESH& mesh) const;

    MESH_LOADER* m_meshLoader;
    GLTF_LOADER* m_gltfLoader;
    std::unordered_map<std::string, VULKAN_MESH> m_meshesMap;
};

extern std::unique_ptr<MESH_MANAGER> pMeshManager;
