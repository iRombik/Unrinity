#pragma once
#include "ecsCommon.h"
#include <queue>
#include <array>

namespace ECS {
    class ENTITY_MANAGER {
    public:
        ENTITY_MANAGER();
        ~ENTITY_MANAGER();
        ENTITY_TYPE CreateEntity();
        void        DestroyEntity(ENTITY_TYPE entityID);

        void                AddComponentToSignature(ENTITY_TYPE entityId, COMPONENT_TYPE componentId);
        void                RemoveComponentFromSignature(ENTITY_TYPE entityId, COMPONENT_TYPE componentId);
        const COMPONENT_SIGNATURE& GetSignature(ENTITY_TYPE entityId) const;

        ENTITY_TYPE GetEntitiesNum() { return m_numEntities; }
    private:
        ENTITY_TYPE                                   m_numEntities;
        std::queue<ENTITY_TYPE>                       m_freeEntityIDs;
        std::array<COMPONENT_SIGNATURE, MAX_ENTITIES> m_entitySignature;
    };
}
