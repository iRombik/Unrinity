#pragma once
#include <bitset>
#include <cstdint>
#include <limits>

namespace ECS {
    using ENTITY_TYPE = uint16_t;
    const ENTITY_TYPE MAX_ENTITIES = 2048;
    static const ENTITY_TYPE INVALID_ENTITY_ID = 128;//std::numeric_limits<ENTITY_TYPE>::max();

    using COMPONENT_TYPE = uint8_t;
    const COMPONENT_TYPE MAX_COMPONENTS = 128;

    using EVENT_TYPE = uint8_t;
    const EVENT_TYPE MAX_EVENTS = 128;

    using COMPONENT_SIGNATURE = std::bitset<MAX_COMPONENTS>;
    using EVENT_SIGNATURE = std::bitset< MAX_EVENTS>;

    using SYSTEM_TYPE       = uint16_t;
    using SYSTEM_PRIORITY_TYPE = uint8_t;
    const SYSTEM_TYPE MAX_SYSTEMS = 128;
}
