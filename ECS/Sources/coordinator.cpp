#include "ecsCoordinator.h"
#include "idGenerator.h"
namespace ECS {
    std::unique_ptr<ECS_COORDINATOR> pEcsCoordinator;

    size_t FAMILY_INDEX_GENERATOR<I_EVENT>::m_count = 0;
    size_t FAMILY_INDEX_GENERATOR<I_SYSTEM>::m_count = 0;
    size_t FAMILY_INDEX_GENERATOR<I_COMPONENT>::m_count = 0;
}
