#pragma once

namespace ECS {
    template<class T>
    class FAMILY_INDEX_GENERATOR
    {
        static size_t m_count;

    public:
        template<class U>
        static const size_t Get()
        {
            static const size_t STATIC_TYPE_ID{ m_count++ };
            return STATIC_TYPE_ID;
        }

        static const size_t Get()
        {
            return m_count;
        }
    };
}
