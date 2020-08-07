#pragma once

#include "ecsCoordinator.h"
#include "geometry.h"

const float TERRAIN_BLOCK_SIZE = 12.f;

struct TERRAIN_BLOCK {
    TERRAIN_BLOCK(glm::vec2 p) : pos(p) {}
    glm::vec2 pos;
};

class TERRAIN_SYSTEM : public ECS::SYSTEM<TERRAIN_SYSTEM>
{
public:
    void Init(glm::vec2 terrainStartPoint, glm::vec2 terrainSize);
    void Update();
    void Render();
private:
    void BeginPass();
    void EndPass();
private:
    glm::vec2 m_terrainStartPoint;
    glm::vec2 m_terrainSize;
    glm::u16vec2 m_terrainBlocksNum;
    std::vector<TERRAIN_BLOCK> m_blocks;
    VULKAN_BUFFER m_indexBuffer;
};
