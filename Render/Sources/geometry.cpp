#include "geometry.h"

std::unique_ptr<VERTEX_DECLARATION_MANAGER> pVertexDeclarationManager;

VERTEX_FORMAT_DESCRIPTOR EMPTY_VERTEX::GetDesc()
{
    return VERTEX_FORMAT_DESCRIPTOR();
}

VERTEX_FORMAT_DESCRIPTOR SIMPLE_VERTEX::GetDesc() {
    VERTEX_FORMAT_DESCRIPTOR formatDesc;
    std::vector<VkVertexInputBindingDescription>& bindingDesctiption = formatDesc.bindingDesctiption;
    bindingDesctiption.resize(1);
    bindingDesctiption[0].binding = 0;
    bindingDesctiption[0].stride = sizeof(SIMPLE_VERTEX);
    bindingDesctiption[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription>& attributeDescription = formatDesc.attributeDescription;
    attributeDescription.resize(3);
    attributeDescription[0].binding = 0;
    attributeDescription[0].location = 0;
    attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescription[0].offset = offsetof(SIMPLE_VERTEX, position);

    attributeDescription[1].binding = 0;
    attributeDescription[1].location = 1;
    attributeDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescription[1].offset = offsetof(SIMPLE_VERTEX, texCoord);
    
    attributeDescription[2].binding = 0;
    attributeDescription[2].location = 2;
    attributeDescription[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescription[2].offset = offsetof(SIMPLE_VERTEX, normal);

    return formatDesc;
}

VERTEX_FORMAT_DESCRIPTOR UI_VERTEX::GetDesc()
{
    VERTEX_FORMAT_DESCRIPTOR formatDesc;
    std::vector<VkVertexInputBindingDescription>& bindingDesctiption = formatDesc.bindingDesctiption;
    bindingDesctiption.resize(1);
    bindingDesctiption[0].binding = 0;
    bindingDesctiption[0].stride = sizeof(UI_VERTEX);
    bindingDesctiption[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription>& attributeDescription = formatDesc.attributeDescription;
    attributeDescription.resize(3);
    attributeDescription[0].binding = 0;
    attributeDescription[0].location = 0;
    attributeDescription[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescription[0].offset = offsetof(UI_VERTEX, pos);

    attributeDescription[1].binding = 0;
    attributeDescription[1].location = 1;
    attributeDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescription[1].offset = offsetof(UI_VERTEX, uv);

    attributeDescription[2].binding = 0;
    attributeDescription[2].location = 2;
    attributeDescription[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    attributeDescription[2].offset = offsetof(UI_VERTEX, col);
    return formatDesc;
}
