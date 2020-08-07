#pragma once
#include "component.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct CAMERA_TRANSFORM_COMPONENT : public ECS::COMPONENT<CAMERA_TRANSFORM_COMPONENT> {
    CAMERA_TRANSFORM_COMPONENT() : position(0.f, 0.f, 0.f), dirLookAt(0.f, 0.f, 0.f) {}
    CAMERA_TRANSFORM_COMPONENT(glm::vec3 _position, glm::vec3 _direction) : position(_position), dirLookAt(_direction) {}

    glm::vec3  position;
    glm::vec3  dirLookAt;
};

struct TRANSFORM_COMPONENT : public ECS::COMPONENT<TRANSFORM_COMPONENT> {
    TRANSFORM_COMPONENT(): position(0.f, 0.f, 0.f) {}
    TRANSFORM_COMPONENT(glm::vec3 _position) : position(_position) {}

    glm::vec3  position;
};

struct ROTATE_COMPONENT : public ECS::COMPONENT<ROTATE_COMPONENT> {
    ROTATE_COMPONENT() : quaternion(glm::vec3(0.f, 0.f, 0.f)) {}
    ROTATE_COMPONENT(glm::fquat _quaternion) : quaternion(_quaternion) {}

    glm::fquat quaternion;
};
