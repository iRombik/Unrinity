#pragma once
#include "support.h"
#include "ecsCoordinator.h"

class GAME_MANAGER {
public:
    bool Init();
    void Run();
    void Term();

    void GenerateSimpleLevel();
    void GeneratePBRTestLevel();
};

extern std::unique_ptr<GAME_MANAGER> pGameManager;
