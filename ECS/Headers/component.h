#pragma once
#include "ecsCommon.h"
#include "idGenerator.h"

namespace ECS {
    struct I_COMPONENT
    {
    };

    template<typename T>
    struct COMPONENT : I_COMPONENT
    {
        static COMPONENT_TYPE GetTypeId() {
            return m_componentTypeId;
        }
        static const COMPONENT_TYPE m_componentTypeId;
    };


    template<class T>
    const COMPONENT_TYPE COMPONENT<T>::m_componentTypeId = (COMPONENT_TYPE)FAMILY_INDEX_GENERATOR<I_COMPONENT>::Get<T>();
};
