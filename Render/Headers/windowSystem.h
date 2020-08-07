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
};

struct SHORT_KEY_STATE {
    std::bitset<4> isButtonPressed; // w, a, s, d
};

struct MOUSE_POSITION {
    MOUSE_POSITION() : x(0), y(0) {}

    int x;
    int y;
};

struct KEY_STATE_EVENT : public ECS::EVENT<KEY_STATE_EVENT>
{
    KEY_STATE_EVENT(SHORT_KEY_STATE&& _keyState) : keyState(_keyState) {}
    SHORT_KEY_STATE keyState;
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
    void UpdateKeyState(int glfwKey);
    SHORT_KEY_STATE CreateShortKeyState();

private:
    KEY_STATE keyStateHolder;
    MOUSE_POSITION mouseStateHolder;
    GLFWwindow * pWindow;
};

extern std::unique_ptr<WINDOW_SYSTEM>      pWindowSystem;
