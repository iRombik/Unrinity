#pragma once
#include "component.h"
#include "glm/glm.hpp"

struct CAMERA_COMPONENT : public ECS::COMPONENT<CAMERA_COMPONENT> {
    float fov;
    float aspectRatio;
    glm::mat4x4 viewProjMatrix;
};

struct INPUT_CONTROLLED : public ECS::COMPONENT<INPUT_CONTROLLED> {
};
