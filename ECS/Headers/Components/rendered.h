#pragma once
#include "component.h"

struct RENDERED_COMPONENT : public ECS::COMPONENT<RENDERED_COMPONENT> {
};

struct VISIBLE_COMPONENT : public ECS::COMPONENT<VISIBLE_COMPONENT> {
};
