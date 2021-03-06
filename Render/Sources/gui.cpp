#include <glm/glm.hpp>
#include <imgui.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>   // for glfwGetWin32Window
#endif

#include "Components/rendered.h"
#include "Components/lightSource.h"

#include "Events/debug.h"

#include "commonRenderVariables.h"
#include "geometry.h"
#include "gui.h"
#include "support.h"
#include "vulkanDriver.h"
#include "windowSystem.h"

#include "resourceSystem.h"
#include "renderTargetManager.h"

static GLFWwindow* g_Window = NULL;    // Main window
static double               g_Time = 0.0;
static bool                 g_MouseJustPressed[ImGuiMouseButton_COUNT] = {};
static GLFWcursor*          g_MouseCursors[ImGuiMouseCursor_COUNT] = {};
static bool                 g_InstalledCallbacks = false;

// Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
static GLFWmousebuttonfun   g_PrevUserCallbackMousebutton = NULL;
static GLFWscrollfun        g_PrevUserCallbackScroll = NULL;
static GLFWkeyfun           g_PrevUserCallbackKey = NULL;
static GLFWcharfun          g_PrevUserCallbackChar = NULL;

struct GUI_SHADER_PUSH_CONST_DATA
{

    int sampledTexId;
};

bool GUI_SYSTEM::Init() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    BindGLFWInterface();

    ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    uint8_t* pixels;
    int width, height;
    int bpp;

    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bpp);

    VULKAN_TEXTURE createdTexture;
    VULKAN_TEXTURE_CREATE_DATA createData;
    createData.pData = pixels;
    createData.dataSize = size_t(width) * height * bpp;
    createData.extent.height = height;
    createData.extent.width = width;
    createData.extent.depth = 1;
    createData.format = VK_FORMAT_R8G8B8A8_UNORM;
    createData.mipLevels = 1;
    createData.mipLevelsOffsets.push_back(0);
    createData.usage = VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    VkResult result = pDrvInterface->CreateTexture(createData, m_fontTexture);
    if (result != VK_SUCCESS) {
        ERROR_MSG("Can't create font texture for UI");
        return false;
    }

    m_mesh.vertexFormatId = UI_VERTEX::formatId;
    ASSERT(sizeof(UI_VERTEX) == sizeof(ImDrawVert));
    VkBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = m_vertexSize * sizeof(UI_VERTEX);
    vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //alloc memory and init buffer
    VkResult vertexBufferCreated =
        pDrvInterface->CreateBuffer(vertexBufferInfo, true, m_mesh.vertexBuffer);
    if (vertexBufferCreated != VK_SUCCESS) {
        ERROR_MSG("Creation vertex buffer for UI failed!");
        return false;
    }

    VkBufferCreateInfo indexBufferInfo = {};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = m_indexSize * sizeof(ImDrawIdx);
    indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult indexBufferCreated =
        pDrvInterface->CreateBuffer(indexBufferInfo, true, m_mesh.indexBuffer);
    if (indexBufferCreated != VK_SUCCESS) {
        ERROR_MSG("Creation index buffer for UI failed!");
        return false;
    }

    m_intermidiateVertexBuffer = new uint8_t[m_vertexSize * sizeof(UI_VERTEX)];
    m_intermidiateIndexBuffer = new uint8_t[m_indexSize * sizeof(ImDrawIdx)];

    m_showLights = false;
    m_debugRT = RT_LAST;

    return true;
}

static void ImGui_ImplGlfw_UpdateMousePosAndButtons()
{
    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
    {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = g_MouseJustPressed[i] || glfwGetMouseButton(g_Window, i) != 0;
        g_MouseJustPressed[i] = false;
    }

    // Update mouse position
    const ImVec2 mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
#ifdef __EMSCRIPTEN__
    const bool focused = true; // Emscripten
#else
    const bool focused = glfwGetWindowAttrib(g_Window, GLFW_FOCUSED) != 0;
#endif
    if (focused)
    {
        if (io.WantSetMousePos)
        {
            glfwSetCursorPos(g_Window, (double)mouse_pos_backup.x, (double)mouse_pos_backup.y);
        }
        else
        {
            double mouse_x, mouse_y;
            glfwGetCursorPos(g_Window, &mouse_x, &mouse_y);
            io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
        }
    }
}

static void ImGui_ImplGlfw_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(g_Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        glfwSetInputMode(g_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else
    {
        // Show OS mouse cursor
        // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
        glfwSetCursor(g_Window, g_MouseCursors[imgui_cursor] ? g_MouseCursors[imgui_cursor] : g_MouseCursors[ImGuiMouseCursor_Arrow]);
        glfwSetInputMode(g_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void GUI_SYSTEM::Update() {
    ImGui_ImplGlfw_UpdateMousePosAndButtons();
    ImGui_ImplGlfw_UpdateMouseCursor();

    //todo: resize windww/render targets event
    ImGuiIO& io = ImGui::GetIO();
    uint32_t widht, height;
    void* pWindowRaw;
    pCallbackGetWindowParams(widht, height, pWindowRaw);
    io.DisplaySize = ImVec2((float)widht, (float)height);
    io.DisplayFramebufferScale = ImVec2(1.f, 1.f);

    ImGui::NewFrame();
    DescribeInterface();
    ImGui::Render();
}

void GUI_SYSTEM::Render() {
    ImDrawData* draw_data = ImGui::GetDrawData();

    if (!draw_data) {
        return;
    }

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) {
        return;
    }

    BeginRenderPass();

    if (draw_data->TotalVtxCount > 0)
    {
        // Create or resize the vertex/index buffers
        const size_t vertexSizeReq = draw_data->TotalVtxCount * sizeof(ImDrawVert);
        const size_t indexSizeReq = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
        //todo: dyn buffer resize?
        ASSERT(m_mesh.vertexBuffer.realBufferSize >= vertexSizeReq);
        ASSERT(m_mesh.indexBuffer.realBufferSize >= indexSizeReq);

        // Upload vertex/index data into a single contiguous GPU buffer
        size_t vertexBufferOffset = 0;
        size_t indexBufferOffset = 0;

        for (int n = 0; n < draw_data->CmdListsCount; n++) {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            const size_t verBufSize = cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
            const size_t indBufSize = cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);

            memcpy(m_intermidiateVertexBuffer + vertexBufferOffset, cmd_list->VtxBuffer.Data, verBufSize);
            memcpy(m_intermidiateIndexBuffer + indexBufferOffset, cmd_list->IdxBuffer.Data, indBufSize);

            vertexBufferOffset += verBufSize;
            indexBufferOffset += indBufSize;
        }
        pDrvInterface->FillBuffer(m_intermidiateVertexBuffer, vertexBufferOffset, 0, m_mesh.vertexBuffer);
        pDrvInterface->FillBuffer(m_intermidiateIndexBuffer, indexBufferOffset, 0, m_mesh.indexBuffer);
    }

    {
        EFFECT_DATA::CB_UI_STRUCT uiData;
        uiData.scale.x = 2.0f / draw_data->DisplaySize.x;
        uiData.scale.y = 2.0f / draw_data->DisplaySize.y;
        uiData.translate.x = -1.0f - draw_data->DisplayPos.x * uiData.scale.x;
        uiData.translate.y = -1.0f - draw_data->DisplayPos.y * uiData.scale.y;
        pDrvInterface->FillConstBuffer(EFFECT_DATA::CB_UI, &uiData, EFFECT_DATA::CONST_BUFFERS_SIZE[EFFECT_DATA::CB_UI]);
    }

    pDrvInterface->SetDynamicState(VK_DYNAMIC_STATE_SCISSOR, true);
    pDrvInterface->SetDepthTestState(false);
    pDrvInterface->SetDepthWriteState(false);

    pDrvInterface->SetShader(EFFECT_DATA::SHR_UI);
    pDrvInterface->SetVertexBuffer(m_mesh.vertexBuffer, 0);
    pDrvInterface->SetIndexBuffer(m_mesh.indexBuffer, 0);
    pDrvInterface->SetVertexFormat(m_mesh.vertexFormatId);
    pDrvInterface->SetTexture(&m_fontTexture, 20);
    pDrvInterface->SetConstBuffer(EFFECT_DATA::CB_UI);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != NULL)
            {
                //not implemented
                ASSERT(false);
                /*
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplVulkan_SetupRenderState(draw_data, command_buffer, rb, fb_width, fb_height);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
                */
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    // Negative offsets are illegal for vkCmdSetScissor
                    if (clip_rect.x < 0.0f)
                        clip_rect.x = 0.0f;
                    if (clip_rect.y < 0.0f)
                        clip_rect.y = 0.0f;

                    // Apply scissor/clipping rectangle
                    VkRect2D scissor;
                    scissor.offset.x = (int32_t)(clip_rect.x);
                    scissor.offset.y = (int32_t)(clip_rect.y);
                    scissor.extent.width = (uint32_t)(clip_rect.z - clip_rect.x);
                    scissor.extent.height = (uint32_t)(clip_rect.w - clip_rect.y);
                    pDrvInterface->SetScissorRect(scissor);

                    int isCustomTex = (pcmd->TextureId != 0);
                    pDrvInterface->FillPushConstantBuffer(&isCustomTex, sizeof(int));

                    // Draw
                    pDrvInterface->DrawIndexed(pcmd->ElemCount, pcmd->VtxOffset + global_vtx_offset, pcmd->IdxOffset + global_idx_offset);
                }
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }
    EndRenderPass();
}

void GUI_SYSTEM::Term()
{
    delete[] m_intermidiateVertexBuffer;
    delete[] m_intermidiateIndexBuffer;

    pDrvInterface->DestroyTexture(m_fontTexture);
    pDrvInterface->DestroyBuffer(m_mesh.vertexBuffer);
    pDrvInterface->DestroyBuffer(m_mesh.indexBuffer);
}

static const char* ImGui_ImplGlfw_GetClipboardText(void* user_data)
{
    return glfwGetClipboardString((GLFWwindow*)user_data);
}

static void ImGui_ImplGlfw_SetClipboardText(void* user_data, const char* text)
{
    glfwSetClipboardString((GLFWwindow*)user_data, text);
}

void ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (g_PrevUserCallbackMousebutton != NULL)
        g_PrevUserCallbackMousebutton(window, button, action, mods);

    if (action == GLFW_PRESS && button >= 0 && button < IM_ARRAYSIZE(g_MouseJustPressed))
        g_MouseJustPressed[button] = true;
}

void ImGui_ImplGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (g_PrevUserCallbackScroll != NULL)
        g_PrevUserCallbackScroll(window, xoffset, yoffset);

    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += (float)xoffset;
    io.MouseWheel += (float)yoffset;
}

void ImGui_ImplGlfw_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (g_PrevUserCallbackKey != NULL)
        g_PrevUserCallbackKey(window, key, scancode, action, mods);

    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;

    // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
#ifdef _WIN32
    io.KeySuper = false;
#else
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
#endif
}

void ImGui_ImplGlfw_CharCallback(GLFWwindow* window, unsigned int c)
{
    if (g_PrevUserCallbackChar != NULL)
        g_PrevUserCallbackChar(window, c);

    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(c);
}

void GUI_SYSTEM::BindGLFWInterface()
{
    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendPlatformName = "imgui_impl_glfw";

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    uint32_t windowWidth, windowHeight;
    void* pWindowRaw;
    pCallbackGetWindowParams(windowWidth, windowHeight, pWindowRaw);
    GLFWwindow* pWindow = static_cast<GLFWwindow*>(pWindowRaw);
    g_Window = pWindow;

    io.SetClipboardTextFn = ImGui_ImplGlfw_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplGlfw_GetClipboardText;
    io.ClipboardUserData = pWindow;
#if defined(_WIN32)
    io.ImeWindowHandle = (void*)glfwGetWin32Window(static_cast<GLFWwindow*>(pWindow));
#endif

    // Create mouse cursors
    // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
    // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
    // Missing cursors will return NULL and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
    GLFWerrorfun prev_error_callback = glfwSetErrorCallback(NULL);
    g_MouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if GLFW_HAS_NEW_CURSORS
    g_MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
#else
    g_MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
#endif
    glfwSetErrorCallback(prev_error_callback);

    // Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
    g_InstalledCallbacks = true;
    g_PrevUserCallbackMousebutton = glfwSetMouseButtonCallback(pWindow, ImGui_ImplGlfw_MouseButtonCallback);
    g_PrevUserCallbackScroll = glfwSetScrollCallback(pWindow, ImGui_ImplGlfw_ScrollCallback);
    g_PrevUserCallbackKey = glfwSetKeyCallback(pWindow, ImGui_ImplGlfw_KeyCallback);
    g_PrevUserCallbackChar = glfwSetCharCallback(pWindow, ImGui_ImplGlfw_CharCallback);
}

void GUI_SYSTEM::DescribeInterface()
{
    ImGui::Begin("Debug window"); 

    static glm::vec4 clearColor = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    //ImGui::ShowDemoWindow();

    const TRANSFORM_COMPONENT* cameraTransform = ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(gameCamera);

    ImGui::BulletText(
        "Camera position = (%.2f, %.2f, %.2f)",
        cameraTransform->position.x, cameraTransform->position.y, cameraTransform->position.z
    );

    if (ImGui::Button("Change draw mode")) {
        gDebugVariables.drawMode = 1 - gDebugVariables.drawMode;
    }

    if (ImGui::Checkbox("Show lights", &m_showLights)) {
        for (auto light : pointLights) {
            if (m_showLights) {
                //ECS::pEcsCoordinator->AddComponentToEntity(light, lightDebugMesh);
                //ECS::pEcsCoordinator->AddComponentToEntity(light, redMaterial);
                ECS::pEcsCoordinator->AddComponentToEntity(light, RENDERED_COMPONENT());
            } else {
                ECS::pEcsCoordinator->RemoveComponentFromEntity<RENDERED_COMPONENT>(light);
                //ECS::pEcsCoordinator->RemoveComponentFromEntity<MESH_COMPONENT>(light);
                //ECS::pEcsCoordinator->RemoveComponentFromEntity<MATERIAL_COMPONENT>(light);
            }
        }
    }

    ImGui::ColorEdit3("Ambient color", (float*)&COMMON_AMBIENT);
    if (ImGui::CollapsingHeader("DirLight")) {
        TRANSFORM_COMPONENT* dirLightTransform = ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(directionalLight);
        DIRECTIONAL_LIGHT_COMPONENT* dirLightComponent = ECS::pEcsCoordinator->GetComponent<DIRECTIONAL_LIGHT_COMPONENT>(directionalLight);

        ImGui::InputFloat3("Position", (float*)&dirLightTransform->position);
        ImGui::SliderFloat("Intensity", &dirLightComponent->intensity, 0.0f, 10.0f);
        ImGui::ColorEdit3("Color", (float*)&dirLightComponent->color);
    }
    if (ImGui::CollapsingHeader("Light0")) {
        TRANSFORM_COMPONENT* lightTransform = ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(pointLights[0]);
        POINT_LIGHT_COMPONENT* lightComponent = ECS::pEcsCoordinator->GetComponent<POINT_LIGHT_COMPONENT>(pointLights[0]);

        ImGui::InputFloat3("Position", (float*)&lightTransform->position);
        ImGui::SliderFloat("Intensity", &lightComponent->intensity, 0.0f, 10.0f);
        ImGui::SliderFloat("Area light", &lightComponent->areaLight, 50.0f, 10000.0f);
        ImGui::ColorEdit3("Color", (float*)&lightComponent->color);
    }
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Dynamic SM depth bias")) {
        ImGui::SliderFloat("Constant", &DEPTH_BIAS_PARAMS.x, 0.0f, 10.0f);
        ImGui::SliderFloat("Slope", &DEPTH_BIAS_PARAMS.y, 0.0f, 10.0f);
    }
    ImGui::Separator();
    if (ImGui::CollapsingHeader("SSAO params")) {
        ImGui::Checkbox("Turn off ssao", &gSSAODebugVariables.turnOffSSAO);
        ImGui::SliderFloat("Bias", &gSSAODebugVariables.bias, 0.001f, 0.5f, "%.3f");
        ImGui::SliderFloat("Radius", &gSSAODebugVariables.radius, 0.1f, 10.0f);
    }
    ImGui::Separator();
    ImGui::Combo("RT debug view", &m_debugRT, RENDER_TARGET_NAME, RENDER_TARGET_ID::RT_LAST + 1);
    if (m_debugRT != RT_LAST) {
        const ImVec2 size(192.f / 9.f * 16.f, 192.f);
        ImGui::Image((void*)0x8, size);
    }

    if (ImGui::Button("Reload shader cache")) 
    {
        pResourceSystem->ReloadShaders();
    }
    ImGui::End();
}

void GUI_SYSTEM::BeginRenderPass()
{
    pRenderTargetManager->SetTextureAsRenderTarget(RT_BACK_BUFFER, 0, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
    if (m_debugRT != RT_LAST && m_debugRT != RT_BACK_BUFFER) {
        pRenderTargetManager->SetTextureAsSRV((RENDER_TARGET_ID)m_debugRT, 21);
    } else {
        pDrvInterface->SetTexture(pResourceSystem->GetDefaultTexture(DEFAULT_RED_TEXTURE), 21);
    }
    pDrvInterface->BeginRenderPass();
}

void GUI_SYSTEM::EndRenderPass()
{
    pDrvInterface->EndRenderPass();
    pRenderTargetManager->ReturnAllRenderTargetsToPool();
}

