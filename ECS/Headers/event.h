#pragma once
#include "ecsCommon.h"

namespace ECS {
    class I_EVENT {
    };

    template<class T>
    class EVENT : public I_EVENT {
    public:
        ~EVENT() {}
        static EVENT_TYPE GetTypeId() { return m_eventTypeId; }
    private:
        static const EVENT_TYPE m_eventTypeId;
    };

    template<class T>
    const EVENT_TYPE EVENT<T>::m_eventTypeId = (EVENT_TYPE)FAMILY_INDEX_GENERATOR<I_EVENT>::Get<T>();
}
