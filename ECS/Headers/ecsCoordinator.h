#pragma once
#include <utility>

#include "support.h"
#include "componentManager.h"
#include "entityManager.h"
#include "systemManager.h"

namespace ECS {
    class ECS_COORDINATOR {
    public:
        template<class S>
        S* CreateSystem() {
            return m_systemMng.RegisterSystem<S>();
        }

        template<class S>
        S* GetSystem() {
            return m_systemMng.GetSystem<S>();
        }

        template<class C>
        void SubscrubeSystemToComponentType(I_SYSTEM* pSystem) {
            m_systemMng.SubscrubeToComponentType(pSystem, C::GetTypeId());
            for (ENTITY_TYPE entityId = 0; entityId < m_entityMng.GetEntitiesNum(); entityId++) {
                m_systemMng.UpdateEntityHandlingForSystem(pSystem, entityId, m_entityMng.GetSignature(entityId));
            }
        }

        template<class E>
        void SubscrubeSystemToEventType(I_SYSTEM* pSystem) {
            m_systemMng.SubscrubeToEventType(pSystem, E::GetTypeId());
        }

        void DestroySystem(I_SYSTEM* pSystem) {
            DEBUG_MSG("DestroySystem() is not implemented!");
        }


        ENTITY_TYPE CreateEntity() {
            return m_entityMng.CreateEntity();
        }

        void DestroyEntity(ENTITY_TYPE entityId) {
            const COMPONENT_SIGNATURE& singature = m_entityMng.GetSignature(entityId);
            m_systemMng.DestroyEntity(entityId);
            m_componentMng.DestroyEntitiesComponents(entityId, singature);
            m_entityMng.DestroyEntity(entityId);
        }


        template<class C>
        void AddComponentToEntity(ENTITY_TYPE entityId, C& componentData) {
            COMPONENT_TYPE componentId = C::GetTypeId();
            m_entityMng.AddComponentToSignature(entityId, componentId);
            m_componentMng.AddComponent(entityId, componentData); // std::forward(componentData)
            m_systemMng.UpdateEntityHandling(entityId, m_entityMng.GetSignature(entityId));
        }

        template<class C>
        void AddComponentToEntity(ENTITY_TYPE entityId, C&& componentData) {
            COMPONENT_TYPE componentId = C::GetTypeId();
            m_entityMng.AddComponentToSignature(entityId, componentId);
            m_componentMng.AddComponent(entityId, std::move(componentData)); // std::forward(componentData)
            m_systemMng.UpdateEntityHandling(entityId, m_entityMng.GetSignature(entityId));
        }

        template<class C>
        C* GetComponent(ENTITY_TYPE entityId) {
            return m_componentMng.GetComponent<C>(entityId);
        }

        template<class C>
        auto GetComponentDataRange() {
            return m_componentMng.GetComponentContainerDataRange<C>();
        }

        template<class C>
        void RemoveComponentFromEntity(ENTITY_TYPE entityId) {
            COMPONENT_TYPE componentId = C::GetTypeId();
            m_entityMng.RemoveComponentFromSignature(entityId, componentId);
            m_componentMng.RemoveComponent<C>(entityId);
            m_systemMng.UpdateEntityHandling(entityId, m_entityMng.GetSignature(entityId));
        }

        template<class E>
        void SendEvent(E&& eventData) const {
            m_systemMng.SendEvent(std::move(eventData));
        }
    private:
        COMPONENT_MANAGER m_componentMng;
        SYSTEM_MANAGER    m_systemMng;
        ENTITY_MANAGER    m_entityMng;
    };

    extern std::unique_ptr<ECS_COORDINATOR> pEcsCoordinator;
};
