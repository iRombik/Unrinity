#include "terrain.h"
#include "vulkanDriver.h"

//tmp
#include "renderTargetManager.h"

void CreateIndexData(std::vector<uint16_t>& indexes) {
    const int numColumns = 4;
    const int numLines = 4;
    for (int i = 0; i < numLines - 1; i++) {
        int offset = i * numColumns;
        for (int j = 0; j < numColumns - 1; j++) {
            int startIndex = j + offset;
            indexes.push_back(startIndex);
            indexes.push_back(startIndex + 1);
            indexes.push_back(startIndex + numColumns);
            indexes.push_back(startIndex + numColumns);
            indexes.push_back(startIndex + 1);
            indexes.push_back(startIndex + numColumns + 1);
        }
    }
}

void TERRAIN_SYSTEM::Init(glm::vec2 terrainStartPoint, glm::vec2 terrainSize)
{
    m_terrainStartPoint = terrainStartPoint;
    m_terrainBlocksNum = terrainSize / TERRAIN_BLOCK_SIZE;
    m_terrainSize.x = m_terrainBlocksNum.x * TERRAIN_BLOCK_SIZE;
    m_terrainSize.y = m_terrainBlocksNum.y * TERRAIN_BLOCK_SIZE;

    for (int w = 0; w < m_terrainBlocksNum.x; w++) {
        for (int h = 0; h < m_terrainBlocksNum.y; h++) {
            glm::vec2 blockPos = m_terrainStartPoint + glm::vec2(w * TERRAIN_BLOCK_SIZE, h * TERRAIN_BLOCK_SIZE);
            m_blocks.emplace_back(blockPos);
        }
    }

    std::vector<uint16_t> indexesRaw;
    CreateIndexData(indexesRaw);
    VkBufferCreateInfo indexBufferInfo = {};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = indexesRaw.size() * sizeof(uint16_t);
    indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    pDrvInterface->CreateAndFillBuffer(indexBufferInfo, (const uint8_t*)indexesRaw.data(), false, m_indexBuffer);
}

void TERRAIN_SYSTEM::Update()
{
}

void TERRAIN_SYSTEM::Render()
{
    BeginPass();
    pDrvInterface->SetShader(EFFECT_DATA::SHR_TERRAIN);

    pDrvInterface->SetDepthTestState(true);
    pDrvInterface->SetDepthWriteState(true);
    pDrvInterface->SetDepthComparitionOperation(true);
    pDrvInterface->SetStencilTestState(false);

    pDrvInterface->SetVertexFormat(EMPTY_VERTEX::formatId);

    EFFECT_DATA::CB_TERRAIN_STRUCT terrainData;
    terrainData.terrainStartPos = m_terrainStartPoint;
    pDrvInterface->FillConstBuffer(EFFECT_DATA::CB_TERRAIN, &terrainData, EFFECT_DATA::CONST_BUFFERS_SIZE[EFFECT_DATA::CB_TERRAIN]);

    //tmp
//     pRenderTargetManager->ReturnRenderTarget(RT_SHADOW_MAP);
//     const VULKAN_TEXTURE* sm = pRenderTargetManager->GetRenderTarget(RT_SHADOW_MAP, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    for (TERRAIN_BLOCK& block : m_blocks) {
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_COMMON_DATA);
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_LIGHTS);
        pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_TERRAIN);

        pDrvInterface->SetTexture(sm, 30);

        pDrvInterface->FillPushConstantBuffer(&block.pos, sizeof(block.pos));

        pDrvInterface->SetIndexBuffer(m_indexBuffer, 0);
        pDrvInterface->DrawIndexed(m_indexBuffer.bufferSize / sizeof(uint16_t));
    }

    EndPass();
}

void TERRAIN_SYSTEM::BeginPass()
{
    pDrvInterface->SetRenderTarget(RT_FP16, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
    pDrvInterface->SetDepthBuffer(RT_DEPTH_BUFFER, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
    pDrvInterface->SetRenderTargetAsShaderResource(RT_SHADOW_MAP, 30);
    pDrvInterface->BeginRenderPass();
}

void TERRAIN_SYSTEM::EndPass()
{
    pDrvInterface->EndRenderPass();
}
