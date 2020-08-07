#include "ecsCommon.h"
#include "entityManager.h"

namespace ECS {
    ENTITY_MANAGER::ENTITY_MANAGER() : m_numEntities(0) {
        for (int id = 0; id < MAX_ENTITIES; id++) {
            m_freeEntityIDs.push(id);
        }
    }

    ENTITY_MANAGER::~ENTITY_MANAGER() {}

    ENTITY_TYPE ENTITY_MANAGER::CreateEntity()
    {
        const ENTITY_TYPE newEntityId = m_freeEntityIDs.front();
        m_freeEntityIDs.pop();
        m_numEntities++;
        return newEntityId;
    }

    void ENTITY_MANAGER::DestroyEntity(ENTITY_TYPE entityId)
    {
        m_entitySignature[entityId].reset();
        m_freeEntityIDs.push(entityId);
        m_numEntities--;
    }

    void ENTITY_MANAGER::AddComponentToSignature(ENTITY_TYPE entityId, COMPONENT_TYPE componentId)
    {
        m_entitySignature[entityId].set(componentId);
    }

    void ENTITY_MANAGER::RemoveComponentFromSignature(ENTITY_TYPE entityId, COMPONENT_TYPE componentId)
    {
        m_entitySignature[entityId].reset(componentId);
    }

    const COMPONENT_SIGNATURE& ENTITY_MANAGER::GetSignature(ENTITY_TYPE entityId) const
    {
        return m_entitySignature[entityId];
    }
}
