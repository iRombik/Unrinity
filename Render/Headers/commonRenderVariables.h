#pragma once
#include <vector>
#include "ecsCommon.h"

static const glm::vec3 UP_VECTOR(0.f, 1.f, 0.f);
static glm::vec3 COMMON_AMBIENT(0.2f);

extern ECS::ENTITY_TYPE gameCamera;
extern std::vector<ECS::ENTITY_TYPE> pointLights;
extern ECS::ENTITY_TYPE directionalLight;
