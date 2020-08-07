#pragma once
#include "support.h"
#include "component.h"

struct POINT_LIGHT_COMPONENT : ECS::COMPONENT< POINT_LIGHT_COMPONENT> {
    float attenuationParam;
    glm::vec3 color;
    float intensity;
    glm::vec3 ambient;
};
