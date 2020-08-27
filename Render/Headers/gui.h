#pragma once
#include "ecsCoordinator.h"
#include "meshManager.h"
#include "GLFW/glfw3.h"

class GUI_SYSTEM : public ECS::SYSTEM<GUI_SYSTEM> {
public:
    bool Init();
    void Update();
    void Render();
    void Term();
private:
    void BindGLFWInterface();
    void DescribeInterface();
    void BeginRenderPass();
    void EndRenderPass();
private:
    VULKAN_TEXTURE m_fontTexture;
    MESH m_mesh;
    //
    bool m_showLights;
    int m_debugRT;

    //tmp thing to avoid non multiple to VkPhysicalDeviceLimits::nonCoherentAtomSize vertex/index offsets
    UINT m_vertexSize = 8096, m_indexSize = 16000;
    uint8_t* m_intermidiateVertexBuffer;
    uint8_t* m_intermidiateIndexBuffer;
};
