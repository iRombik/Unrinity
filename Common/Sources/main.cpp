#include <windows.h>
#include "game.h"

int WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    pGameManager.reset(new GAME_MANAGER());

    if (pGameManager->Init()) {
        pGameManager->Run();
    }
    pGameManager->Term();
    return 0;
}
