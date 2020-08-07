#pragma once
#include "event.h"

struct UPDATE_CAMERA_MATRIX_EVENT : public ECS::EVENT< UPDATE_CAMERA_MATRIX_EVENT>
{
    UPDATE_CAMERA_MATRIX_EVENT(ECS::ENTITY_TYPE _entityId) :entityId(_entityId) {}
    ECS::ENTITY_TYPE entityId;
};