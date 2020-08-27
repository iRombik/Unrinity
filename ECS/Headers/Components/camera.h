#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "component.h"
#include "glm/glm.hpp"

struct CAMERA_COMPONENT : public ECS::COMPONENT<CAMERA_COMPONENT> {
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    glm::mat4x4 viewMatrix;
    glm::mat4x4 projMatrix;
    glm::mat4x4 viewProjMatrix;
};

struct INPUT_CONTROLLED : public ECS::COMPONENT<INPUT_CONTROLLED> {
};
