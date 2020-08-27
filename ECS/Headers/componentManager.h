#pragma once
#include "ecsCommon.h"
#include "component.h"
#include <unordered_map>

namespace ECS {
    class I_COMPONENT_CONTAINER
    {
    public:
        virtual const char* GetTypeName() const = 0;
        virtual void DestroyComponent(ENTITY_TYPE entity) = 0;
        virtual ~I_COMPONENT_CONTAINER() {};
    };

    template<typename T>
    class COMPONENT_CONTAINER : public I_COMPONENT_CONTAINER
    {
        static const size_t INVALID_COMPONENT_ID = MAX_ENTITIES;//std::numeric_limits<size_t>::max();
    public:
        COMPONENT_CONTAINER() : m_curSize(0) 
        {
            m_entityToIndexMap.fill(INVALID_COMPONENT_ID);
            m_indexToEntityMap.fill(INVALID_ENTITY_ID);
        }

        ~COMPONENT_CONTAINER() {
        }

        const char* GetTypeName() const override {
            static const char* typeName{ typeid(T).name() };
            return typeName;
        }

        void InsertData(ENTITY_TYPE entity, T& component) {
            ASSERT(entity < MAX_ENTITIES);
            size_t index = m_entityToIndexMap[entity];
            if (index == INVALID_COMPONENT_ID) {
                index = m_curSize++;
                m_entityToIndexMap[entity] = index;
                m_indexToEntityMap[index] = entity;
            }
            m_componentArray[index] = std::move(component);
        }

        void InsertData(ENTITY_TYPE entity, T&& component) {
            ASSERT(entity < MAX_ENTITIES);
            size_t index = m_entityToIndexMap[entity];
            if (index == INVALID_COMPONENT_ID) {
                index = m_curSize++;
                m_entityToIndexMap[entity] = index;
                m_indexToEntityMap[index] = entity;
            }
            m_componentArray[index] = std::move(component);
        }

        void RemoveData(ENTITY_TYPE entity) {
            ASSERT(entity < MAX_ENTITIES);
            const size_t removedComponentId = m_entityToIndexMap[entity];
            ASSERT_MSG(removedComponentId != INVALID_COMPONENT_ID, "Trying to remove component, that doesn't belongs to entity");
            const size_t lastElementId = --m_curSize;
            const ENTITY_TYPE entityOflastElement = m_indexToEntityMap[lastElementId];
            m_entityToIndexMap[entityOflastElement] = INVALID_COMPONENT_ID;
            m_indexToEntityMap[lastElementId] = INVALID_ENTITY_ID;

            if (!m_curSize || removedComponentId == lastElementId) {
                return;
            }

            //we want to avoid holes in array to abuse cache
            //put last element data to the place of removed object and update maps
            m_componentArray[removedComponentId] = m_componentArray[lastElementId];
            m_entityToIndexMap[entityOflastElement] = removedComponentId;
            m_indexToEntityMap[removedComponentId] = entityOflastElement;
        }

        T* GetData(ENTITY_TYPE entity) {
            ASSERT(entity < MAX_ENTITIES);
            const size_t componentId = m_entityToIndexMap[entity];
            return componentId == INVALID_COMPONENT_ID ? nullptr : &m_componentArray[componentId];
        }

        auto GetDataRange() const {
            std::pair<std::array<T>::iterator, std::array<T>::iterator> p(m_componentArray.begin(), m_componentArray.at(m_curSize));
            return p;
        }

        void DestroyComponent(ENTITY_TYPE entity) override {
            RemoveData(entity);
        }

        static COMPONENT_TYPE GetTypeId() {
            return T::GetTypeId();
        }
    private:
        size_t                                    m_curSize;
        std::array<T, MAX_ENTITIES>               m_componentArray;
        std::array<size_t, MAX_ENTITIES>          m_entityToIndexMap;
        std::array<ENTITY_TYPE, MAX_ENTITIES>     m_indexToEntityMap;
    };

    class COMPONENT_MANAGER {
    public:
        COMPONENT_MANAGER() {}

        ~COMPONENT_MANAGER() {
            for (auto& componentContainer : m_componentRegistryList) {
                SAFE_DELETE(componentContainer.second);
            }
            m_componentRegistryList.clear();
        }

        template<class T>
        void AddComponent(ENTITY_TYPE entity, T& component) {
            GetComponentContainer<T>()->InsertData(entity, component);
        }

        template<class T>
        void AddComponent(ENTITY_TYPE entity, T&& component) {
            GetComponentContainer<T>()->InsertData(entity, std::move(component));
        }

        template<class T>
        void RemoveComponent(ENTITY_TYPE entity) {
            GetComponentContainer<T>()->RemoveData(entity);
        }

        template<class T>
        auto GetComponentContainerDataRange() {
            GetComponentContainer<T>()->GetDataRange();
        }

        template<class T>
        T* GetComponent(ENTITY_TYPE entity) {
            return GetComponentContainer<T>()->GetData(entity);
        }
        template<class T>
        const T& GetConstComponent(ENTITY_TYPE entity) const {
            return GetComponentContainer<T>()->GetData(entity);
        }

        void DestroyEntitiesComponents(ENTITY_TYPE entity, const COMPONENT_SIGNATURE& entitySignature) {
            for (auto& componentContainer: m_componentRegistryList) {
                if (entitySignature.test(componentContainer.first)) {
                    componentContainer.second->DestroyComponent(entity);
                }
            }
        }

        template<class T>
        const char* GetTypeName() const {
            return GetComponentContainer<T>()->GetTypeName();
        }
    private:
        COMPONENT_MANAGER(const COMPONENT_MANAGER& cmpMng) = delete;
        COMPONENT_MANAGER& operator=(const COMPONENT_MANAGER& cmpMng) = delete;

        template<class T>
        COMPONENT_CONTAINER<T>* GetComponentContainer() {
            const COMPONENT_TYPE componentId = T::GetTypeId();
            auto componentContainer = m_componentRegistryList.find(componentId);
            if (componentContainer == m_componentRegistryList.end()) {
                m_componentRegistryList.emplace(componentId, new COMPONENT_CONTAINER<T>());
                componentContainer = m_componentRegistryList.find(componentId);
            } 
            return static_cast<COMPONENT_CONTAINER<T>*>(componentContainer->second);
        }

        std::unordered_map<COMPONENT_TYPE, I_COMPONENT_CONTAINER*> m_componentRegistryList;
    };
};
