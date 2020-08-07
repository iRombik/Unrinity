#pragma once
#include <vector>
#include <unordered_set>
#include "ecsCommon.h"
#include "event.h"

namespace ECS {
    class I_SYSTEM {
    public:
        virtual const char* GetTypeName() const = 0;

        virtual const EVENT_SIGNATURE&     GetEventSignature      () const = 0;
        virtual const COMPONENT_SIGNATURE& GetComponentsSignature () const = 0;

        virtual bool IsEntityHandling (ENTITY_TYPE entity) const       = 0;
        virtual void AddEntity        (ENTITY_TYPE entityId)           = 0;
        virtual void RemoveEntity     (ENTITY_TYPE entityId)           = 0;

        virtual void SubscrubeToComponentType     (COMPONENT_TYPE componentType) = 0;

        virtual void SubscrubeToEventType          (EVENT_TYPE eventType) = 0;
        virtual void UnsubscrubeFromEventType      (EVENT_TYPE eventType) = 0;

        virtual void SendEvent(EVENT_TYPE eventType, std::shared_ptr<I_EVENT>& eventPtr) = 0;

        //virtual void Update() = 0;
    };

    template<class T>
    class SYSTEM : public I_SYSTEM
    {
    public:
        SYSTEM() {};
        ~SYSTEM() {};

        static SYSTEM_TYPE GetTypeId() {
            return m_systemTypeId;
        }

        const char* GetTypeName() const override {
            static const char* typeName{ typeid(T).name() };
            return typeName;
        }

        bool IsEntityHandling(ENTITY_TYPE entity) const override {
            return m_entityList.find(entity) != m_entityList.end();
        }
        void AddEntity(ENTITY_TYPE entityId) override {
            m_entityList.insert(entityId);
        }
        void RemoveEntity(ENTITY_TYPE entityId) override {
            m_entityList.erase(entityId);
        }

        const EVENT_SIGNATURE& GetEventSignature() const override {
            return m_eventSignature;
        }

        const COMPONENT_SIGNATURE& GetComponentsSignature() const override {
            return m_componentSignature;
        }

        void SubscrubeToComponentType(COMPONENT_TYPE componentType) override {
            m_componentSignature.set(componentType);
        }

        void SubscrubeToEventType(EVENT_TYPE eventType) override {
            m_eventSignature.set(eventType);
        }

        void UnsubscrubeFromEventType(EVENT_TYPE eventType) override {
            m_eventSignature.reset(eventType);
        }

        void SendEvent(EVENT_TYPE eventType, std::shared_ptr<I_EVENT>& eventPtr) override {
            m_eventList.emplace(eventType, eventPtr);
        }

    protected:
        template<class E>
        bool IsEventList() const {
            return  m_eventList.find(E::GetTypeId()) != m_eventList.end();
        }

        template<class E>
        auto GetEventList () {
            return m_eventList.equal_range(E::GetTypeId());
        }

        void ClearEventList() {
            m_eventList.clear();
        }

    protected:
        static const SYSTEM_TYPE              m_systemTypeId;

        COMPONENT_SIGNATURE                   m_componentSignature;
        std::unordered_set<ENTITY_TYPE>       m_entityList;

        EVENT_SIGNATURE                                               m_eventSignature;
        std::unordered_multimap<EVENT_TYPE, std::shared_ptr<I_EVENT>> m_eventList;
    };

    template<class T>
    const SYSTEM_TYPE SYSTEM<T>::m_systemTypeId = (SYSTEM_TYPE) FAMILY_INDEX_GENERATOR<I_SYSTEM>::Get<T>();

    class SYSTEM_MANAGER {
    public:
        template<class T>
        T* RegisterSystem() {
            T* createdSystem = new T();
            ASSERT(m_systemRegistryList.find(T::GetTypeId()) == m_systemRegistryList.end());
            m_systemRegistryList[T::GetTypeId()] = createdSystem;
            return createdSystem;
        }

        template<class T>
        T* GetSystem() {
            return static_cast<T*>(m_systemRegistryList[T::GetTypeId()]);
        }

        void SubscrubeToComponentType(I_SYSTEM* pSystem, COMPONENT_TYPE componentType) {
            pSystem->SubscrubeToComponentType(componentType);
        }

        void SubscrubeToEventType(I_SYSTEM* pSystem, EVENT_TYPE eventType) {
            m_subscrubesTable.emplace(eventType, pSystem);
            pSystem->SubscrubeToEventType(eventType);
        }

        void UnsubscrubeFromEventType(I_SYSTEM* pSystem, EVENT_TYPE eventType) {
            auto subscruberRange = m_subscrubesTable.equal_range(eventType);
            for (auto iter = subscruberRange.first; iter != subscruberRange.second; iter++) {
                if (iter->second == pSystem) {
                    pSystem->UnsubscrubeFromEventType(eventType);
                    m_subscrubesTable.erase(iter);
                    return;
                }
            }
        }
        /*
        template<class E, class... ARGS>
        void SendEvent(ARGS&&...eventArgs) const {
            std::shared_ptr<E> eventPtr = std::make_shared<E>(eventArgs);
            const EVENT_TYPE eventType = E::GetTypeId();
            auto subscruberRange = m_subscrubesTable.equal_range(eventType);
            for (auto iter = subscruberRange.first; iter != subscruberRange.second; iter++) {
                iter->second->SendEvent(eventPtr);
            }
        }
        */
        template<class E>
        void SendEvent(E&& eventData) const {
            std::shared_ptr<I_EVENT> eventPtr(new E(eventData));
            const EVENT_TYPE eventType = E::GetTypeId();
            auto subscruberRange = m_subscrubesTable.equal_range(eventType);
            for (auto iter = subscruberRange.first; iter != subscruberRange.second; iter++) {
                iter->second->SendEvent(eventType, eventPtr);
            }
        }

        void DestroyEntity (ENTITY_TYPE entityId) const {
            for (auto& [systemId, pSystem]: m_systemRegistryList) {
                pSystem->RemoveEntity(entityId);
            }
        }

        void UpdateEntityHandling (ENTITY_TYPE entityId, const COMPONENT_SIGNATURE& entitySignature) const {
            for (auto& [systemId, pSystem] : m_systemRegistryList) {
                UpdateEntityHandlingForSystem(pSystem, entityId, entitySignature);
            }
        }


        void UpdateEntityHandlingForSystem(I_SYSTEM* pSystem, ENTITY_TYPE entityId, const COMPONENT_SIGNATURE& entitySignature) const {
            const COMPONENT_SIGNATURE& sysSignature = pSystem->GetComponentsSignature();
            bool isEntityShouldHandled = (sysSignature & entitySignature) == sysSignature;
            if (isEntityShouldHandled) {
                pSystem->AddEntity(entityId);
            } else {
                pSystem->RemoveEntity(entityId);
            }
        }
    private:
        std::unordered_map<SYSTEM_TYPE, I_SYSTEM*>     m_systemRegistryList;
        std::unordered_multimap<EVENT_TYPE, I_SYSTEM*> m_subscrubesTable;
    };
}
