[[vk::binding(2)]]
cbuffer terrainCommon : register (b2) {
    float2 terrainStartPos;
};

static const float TERRAIN_BLOCK_SIZE = 12.f;
static const float TERRAIN_VERTEX_PER_COLUMN_LINE = 4.f;
static const float TERRAIN_VERTEX_OFFSET = TERRAIN_BLOCK_SIZE / (TERRAIN_VERTEX_PER_COLUMN_LINE - 1.f);
