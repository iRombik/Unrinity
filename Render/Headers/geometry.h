#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "idGenerator.h"
#include "vulkanDriver.h"

struct VERTEX_FORMAT_DESCRIPTOR {
    std::vector<VkVertexInputBindingDescription>   bindingDesctiption;
    std::vector<VkVertexInputAttributeDescription> attributeDescription;
};

template<class T>
struct VERTEX_FORMAT_IDENTIFIER {
    static uint8_t formatId;
};

struct EMPTY_VERTEX : public VERTEX_FORMAT_IDENTIFIER<EMPTY_VERTEX>
{
    static VERTEX_FORMAT_DESCRIPTOR GetDesc();
};

struct SIMPLE_VERTEX : public VERTEX_FORMAT_IDENTIFIER<SIMPLE_VERTEX>
{
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec3 normal;

    static VERTEX_FORMAT_DESCRIPTOR GetDesc();
};

struct UI_VERTEX : public VERTEX_FORMAT_IDENTIFIER<UI_VERTEX> //, public ImDrawVert
{
    glm::vec2  pos;
    glm::vec2  uv;
    uint32_t   col;

    static VERTEX_FORMAT_DESCRIPTOR GetDesc();
};

template<class T>
uint8_t VERTEX_FORMAT_IDENTIFIER<T>::formatId = UINT8_MAX;

class VERTEX_DECLARATION_MANAGER {
public:
    void Init() {
        uint8_t formatIdCounter = 0;
        RegisterFormat<EMPTY_VERTEX>(formatIdCounter);
        RegisterFormat<SIMPLE_VERTEX>(formatIdCounter);
        RegisterFormat<UI_VERTEX>(formatIdCounter);
    }

    const VERTEX_FORMAT_DESCRIPTOR& GetDesc(uint8_t formatId) const {
        return m_descriptors[formatId];
    }
private:
    template<class T>
    void RegisterFormat(uint8_t& formatIdCounter) {
        m_descriptors[formatIdCounter] = T::GetDesc();
        T::formatId = formatIdCounter++;
    }
    std::array<VERTEX_FORMAT_DESCRIPTOR, 3> m_descriptors;
};


extern std::unique_ptr<VERTEX_DECLARATION_MANAGER> pVertexDeclarationManager;
//template <class v, class I>
struct RAW_MESH
{
    std::vector<SIMPLE_VERTEX> vertexes;
    std::vector<uint16_t> indexes;
};
