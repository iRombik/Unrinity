#include "meshManager.h"
#include "support.h"
#include "geometry.h"
#include "vulkanDriver.h"

#include <vector>
#include <fstream> 
#include <map>

std::unique_ptr<MESH_MANAGER> pMeshManager;

class MESH_LOADER
{
public:
    RAW_MESH* LoadMesh(const std::string& meshName);
private:
    struct VERTEX_DATA_ID_BATCH
    {
        bool operator<(const VERTEX_DATA_ID_BATCH& vdb) const {
            if (posId != vdb.posId) {
                return posId < vdb.posId;
            }
            if (texCoordId != vdb.texCoordId) {
                return texCoordId < vdb.texCoordId;
            }
            return normalId < vdb.normalId;
        }
        VERTEX_DATA_ID_BATCH() : posId(0), texCoordId(0), normalId(0){}
        int32_t posId, texCoordId, normalId;
    };

    void ClearState();

    void SkipLine();
    void AddPosition();
    void AddTexCoord();
    void AddNormal();
    void AddFaces();
    void ReadMaterial();
    void StartNewObject();

    void PushDataToMesh(VERTEX_DATA_ID_BATCH dataBatch);

    std::vector<glm::vec3> m_position;
    std::vector<glm::vec2> m_texCoords;
    std::vector<glm::vec3> m_normals;

    const glm::vec3 DEFAULT_POSITION   = glm::vec3(0, 0, 0);
    const glm::vec2 DEFAULT_TEX_COORD = glm::vec2(0, 0);
    const glm::vec3 DEFAULT_NORMAL    = glm::vec3(0, 0, 0);

    std::map<VERTEX_DATA_ID_BATCH, uint16_t> m_vertexToIndexMap;

    std::ifstream m_meshFile;
    RAW_MESH* m_resultMesh;
};

RAW_MESH* MESH_LOADER::LoadMesh(const std::string& meshPath)
{
    ClearState();

    m_meshFile.open(meshPath, std::fstream::in | std::ios::binary);
    if (!m_meshFile.is_open()) {
        WARNING_MSG(formatString("Can't open %s mesh file!", meshPath.c_str()).c_str());
        return nullptr;
    }

    m_resultMesh = new RAW_MESH;
    std::string cmd;
    while (m_meshFile >> std::skipws >> cmd) {
        if (cmd == "#") {
            SkipLine();
        } else if (cmd == "v") {
            AddPosition();
        } else if (cmd == "vt") {
            AddTexCoord();
        } else if (cmd == "vn") {
            AddNormal();
        } else if (cmd == "f") {
            AddFaces();
        } else if (cmd == "mtllib") {
            ReadMaterial();
        } else if (cmd == "o") {
            StartNewObject();
        } else {
            WARNING_MSG(formatString("Unknown .obj command: %s!\n ", cmd.c_str()).c_str());
            SkipLine();
        }
    }
    m_meshFile.close();
    return m_resultMesh;
}

void MESH_LOADER::ClearState()
{
    m_position.clear();
    m_texCoords.clear();
    m_normals.clear();
    m_vertexToIndexMap.clear();
    m_meshFile.close();
    m_resultMesh = nullptr;
}

void MESH_LOADER::SkipLine()
{
    char c;
    m_meshFile >> std::noskipws;
    while ((m_meshFile >> c) && (c !='\n'));
}

void MESH_LOADER::AddPosition()
{
    glm::vec3 pos;
    m_meshFile >> pos.x >> pos.y >> pos.z;
    m_position.push_back(pos);
}

void MESH_LOADER::AddTexCoord()
{
    glm::vec2 texCoord;
    m_meshFile >> texCoord.x >> texCoord.y;
    m_texCoords.push_back(texCoord);
}

void MESH_LOADER::AddNormal()
{
    glm::vec3 norm;
    m_meshFile >> norm.x >> norm.y >> norm.z;
    m_normals.push_back(norm);
}

void MESH_LOADER::PushDataToMesh(VERTEX_DATA_ID_BATCH vdb)
{
    auto iter = m_vertexToIndexMap.find(vdb);
    if (iter == m_vertexToIndexMap.end()) {
        uint16_t vertexId = static_cast<uint16_t>(m_vertexToIndexMap.size());
        SIMPLE_VERTEX vertexData;
        vertexData.position = m_position[vdb.posId];
        if (vdb.texCoordId == -1) {
            vertexData.texCoord = DEFAULT_TEX_COORD;
        } else {
            vertexData.texCoord = m_texCoords[vdb.texCoordId];
        }
        if (vdb.normalId == -1) {
            vertexData.normal = DEFAULT_NORMAL;
        } else {
            vertexData.normal = m_normals[vdb.normalId];
        }
        m_resultMesh->vertexes.push_back(vertexData);
        m_resultMesh->indexes.push_back(vertexId);
        m_vertexToIndexMap[vdb] = vertexId;
    } else {
        m_resultMesh->indexes.push_back(iter->second);
    }
}

void MESH_LOADER::AddFaces()
{
    int faceVertId = 0;
    m_meshFile >> std::noskipws;
    while (m_meshFile.peek() != '\r' && m_meshFile.peek() != '\n') {
        ASSERT(faceVertId < 3);
        VERTEX_DATA_ID_BATCH vdb;
        char c;
        m_meshFile >> c >> vdb.posId;
        ASSERT(c == ' ');
        if (m_meshFile.peek() == '/')
        {
            m_meshFile.ignore();

            if (m_meshFile.peek() != '/') {
                m_meshFile >> vdb.texCoordId;
            }

            if (m_meshFile.peek() == '/')
            {
                m_meshFile.ignore();
                m_meshFile >> vdb.normalId;
            }
        }
        vdb.posId--;
        vdb.texCoordId--;
        vdb.normalId--;
        PushDataToMesh(vdb);
        faceVertId++;
    }
}

void MESH_LOADER::ReadMaterial()
{
    SkipLine();
}

void MESH_LOADER::StartNewObject()
{
}

MESH_MANAGER::MESH_MANAGER() : m_meshLoader(new MESH_LOADER)
{
}

MESH_MANAGER::~MESH_MANAGER()
{
    Term();
    delete m_meshLoader;
}

void MESH_MANAGER::Init()
{
    m_meshesMap.clear();
}

void MESH_MANAGER::LoadCommonMeshes()
{
    LoadMesh("cube");
    LoadMesh("sphere");
}

bool MESH_MANAGER::LoadMesh(const std::string& meshName)
{
    const std::string meshPath = GetMeshPath(meshName);
    const RAW_MESH* pRawMesh = m_meshLoader->LoadMesh(meshPath);

    if (!pRawMesh) {
        WARNING_MSG(formatString("Mesh %s wan't loaded", meshName.c_str()).c_str());
        return false;
    }

    MESH resultMesh;

    resultMesh.vertexFormatId = SIMPLE_VERTEX::formatId;
    resultMesh.numOfVertexes = (uint32_t)pRawMesh->vertexes.size();
    resultMesh.numOfIndexes = (uint32_t)pRawMesh->indexes.size();

    VkBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = pRawMesh->vertexes.size() * sizeof(SIMPLE_VERTEX);
    vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //alloc memory and init buffer
    VkResult vertexBufferCreated =
        pDrvInterface->CreateAndFillBuffer(vertexBufferInfo, (const uint8_t*)pRawMesh->vertexes.data(), false, resultMesh.vertexBuffer);

    VkBufferCreateInfo indexBufferInfo = {};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = pRawMesh->indexes.size() * sizeof(uint16_t);
    indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult indexBufferCreated =
        pDrvInterface->CreateAndFillBuffer(indexBufferInfo, (const byte*)pRawMesh->indexes.data(), false, resultMesh.indexBuffer);

    SAFE_DELETE(pRawMesh);
    if (vertexBufferCreated != VK_SUCCESS || indexBufferCreated != VK_SUCCESS) {
        ASSERT_MSG(vertexBufferCreated == VK_SUCCESS, "Vertex buffer not created!");
        ASSERT_MSG(indexBufferCreated == VK_SUCCESS, "Index buffer not created!");
        DestroyMesh(resultMesh);
        return false;
    }

    m_meshesMap.emplace(meshName, resultMesh);
    return true;
}

void MESH_MANAGER::Term()
{
    for (auto iter : m_meshesMap) {
        DestroyMesh(iter.second);
    }
}

MESH* MESH_MANAGER::GetMeshByName(const std::string& meshName)
{
    auto iter = m_meshesMap.find(meshName);
    if (iter == m_meshesMap.end()) {
        bool isMeshLoaded = LoadMesh(meshName);
        if (!isMeshLoaded) {
            return nullptr;
        }
        iter = m_meshesMap.find(meshName);
    }
    return &iter->second;
}

std::string MESH_MANAGER::GetMeshPath(const std::string& meshName)
{
    return MESH_DIR + meshName + "/" + meshName + MESH_EXT;
}

void MESH_MANAGER::DestroyMesh (MESH& mesh) const {
    pDrvInterface->DestroyBuffer(mesh.vertexBuffer);
    pDrvInterface->DestroyBuffer(mesh.indexBuffer);
}
