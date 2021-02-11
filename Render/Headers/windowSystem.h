#pragma once
#include <GLFW/glfw3.h>
#include "ecsCoordinator.h"

struct WINDOW_STATE : public ECS::COMPONENT<WINDOW_STATE>
{
    WINDOW_STATE() :width(0), height(0), pWindow(nullptr) {}

    uint32_t    width;
    uint32_t    height;
    GLFWwindow* pWindow;
};

struct WINDOW_SYSTEM {
    bool Init();
    bool Update();
    void Term();

    WINDOW_SYSTEM() : windowState() {};
    WINDOW_SYSTEM(const WINDOW_SYSTEM&) = delete;
    WINDOW_SYSTEM& operator=(const WINDOW_SYSTEM&) = delete;
    ~WINDOW_SYSTEM() { Term(); }

private:
    WINDOW_STATE windowState;
};

struct KEY_STATE {
    std::bitset<GLFW_KEY_LAST + 1> isButtonPressed;
    std::bitset<GLFW_KEY_LAST + 1> isButtonWasPressed;
};

struct KEY_STATE_EVENT : ECS::EVENT<KEY_STATE_EVENT> {
    const KEY_STATE* keyState;
};

struct MOUSE_POSITION {
    MOUSE_POSITION() : x(0), y(0) {}

    int x;
    int y;
};

struct MOUSE_STATE_EVENT : ECS::EVENT<MOUSE_STATE_EVENT>
{
    MOUSE_POSITION mouseState;
    MOUSE_POSITION mouseShift;
};

class INPUT_SYSTEM {
public:
    void Init(GLFWwindow* pWindow);
    void Update();
private:
    bool UpdateKeyState(int glfwKey);

private:
    KEY_STATE keyStateHolder;
    MOUSE_POSITION mouseStateHolder;
    GLFWwindow * pWindow;
};

extern std::unique_ptr<WINDOW_SYSTEM>      pWindowSystem;
