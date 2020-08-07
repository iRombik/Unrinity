#include <memory>
#include "windowSystem.h"
#include "vulkanDriver.h"

std::unique_ptr<WINDOW_SYSTEM>  pWindowSystem;
std::unique_ptr<INPUT_SYSTEM>   pInputSystem;

bool WINDOW_SYSTEM::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    windowState.width = 1600;
    windowState.height = 900;
    windowState.pWindow = glfwCreateWindow(windowState.width, windowState.height, "Main window", NULL, nullptr);

    pInputSystem.reset(new INPUT_SYSTEM);
    pInputSystem->Init(windowState.pWindow);

    pCallbackGetWindowParams = [&](uint32_t& width, uint32_t& height, void*& pWindow) {
        width = windowState.width;
        height = windowState.height;
        pWindow = (void*)windowState.pWindow;
    };
    glfwFocusWindow(windowState.pWindow);

    return windowState.pWindow != NULL;
}

bool WINDOW_SYSTEM::Update()
{
    glfwPollEvents();
    pInputSystem->Update();
    return !glfwWindowShouldClose(windowState.pWindow);
}

void WINDOW_SYSTEM::Term()
{
    glfwDestroyWindow(windowState.pWindow);
    glfwTerminate();
}


void INPUT_SYSTEM::Init(GLFWwindow* _pWindow)
{
    pWindow = _pWindow;

    double xPos, yPos;
    glfwGetCursorPos(pWindow, &xPos, &yPos);
    mouseStateHolder.x = (int)xPos;
    mouseStateHolder.y = (int)yPos;
}

void INPUT_SYSTEM::Update()
{
    UpdateKeyState(GLFW_KEY_W);
    UpdateKeyState(GLFW_KEY_A);
    UpdateKeyState(GLFW_KEY_S);
    UpdateKeyState(GLFW_KEY_D);
    UpdateKeyState(GLFW_KEY_LEFT_CONTROL);
    UpdateKeyState(GLFW_KEY_TAB);

    ECS::pEcsCoordinator->SendEvent(KEY_STATE_EVENT(CreateShortKeyState()));

    double xPos, yPos;
    glfwGetCursorPos(pWindow, &xPos, &yPos);

    MOUSE_STATE_EVENT mse;
    mse.mouseShift.x = (int)xPos - mouseStateHolder.x;
    mse.mouseShift.y = (int)yPos - mouseStateHolder.y;
    mse.mouseState.x = (int)xPos;
    mse.mouseState.y = (int)yPos;
    mouseStateHolder = mse.mouseState;

    if (keyStateHolder.isButtonPressed[GLFW_KEY_LEFT_CONTROL]) {
        ECS::pEcsCoordinator->SendEvent(MOUSE_STATE_EVENT(mse));
    }
}

void INPUT_SYSTEM::UpdateKeyState(int glfwKey)
{
    int state = glfwGetKey(pWindow, glfwKey);
    if (state == GLFW_PRESS) {
        keyStateHolder.isButtonPressed.set(glfwKey, true);
    }
    if (state == GLFW_RELEASE) {
        keyStateHolder.isButtonPressed.set(glfwKey, false);
    }
}

SHORT_KEY_STATE INPUT_SYSTEM::CreateShortKeyState()
{
    SHORT_KEY_STATE result;
    result.isButtonPressed[0] = keyStateHolder.isButtonPressed[GLFW_KEY_W];
    result.isButtonPressed[1] = keyStateHolder.isButtonPressed[GLFW_KEY_A];
    result.isButtonPressed[2] = keyStateHolder.isButtonPressed[GLFW_KEY_S];
    result.isButtonPressed[3] = keyStateHolder.isButtonPressed[GLFW_KEY_D];
    return result;
}

