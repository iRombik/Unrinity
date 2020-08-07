#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "vulkanDriver.h"
#include "componentManager.h"

class MESH_LOADER;
class MESH_MANAGER;

struct MESH
{
    uint8_t vertexFormatId;
    VULKAN_BUFFER vertexBuffer;
    VULKAN_BUFFER indexBuffer;
    uint32_t numOfVertexes;
    uint32_t numOfIndexes;
};

struct MESH_COMPONENT : public ECS::COMPONENT<MESH>
{
    const MESH* pMesh;
};

class MESH_MANAGER {
public:
    MESH_MANAGER();
    ~MESH_MANAGER();

    void Init();
    void LoadCommonMeshes();
    void Term();

    MESH* GetMeshByName(const std::string& meshName);
private:
    const std::string MESH_DIR = "../Media/Meshes/";
    const std::string MESH_EXT = ".obj";

    std::string GetMeshPath(const std::string& meshName);

    bool  LoadMesh(const std::string& meshName);
    void  DestroyMesh (MESH& mesh) const;

    MESH_LOADER* m_meshLoader;
    std::unordered_map<std::string, MESH> m_meshesMap;
};

extern std::unique_ptr<MESH_MANAGER> pMeshManager;
