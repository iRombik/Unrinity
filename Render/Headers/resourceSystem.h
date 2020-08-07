#pragma once
#include <memory>

struct RESOURCE_SYSTEM {
    void Init();
    void Reload();
    void Term();
};

extern std::unique_ptr<RESOURCE_SYSTEM> pResourceSystem;