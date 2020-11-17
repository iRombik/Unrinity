#pragma once
#include <memory>
#include <string>

class RESOURCE_SYSTEM {
public:
    void Init();
    void Reload();
    void Term();

    bool LoadShaders();
    void ReloadShaders();

    bool LoadScene(const std::string& levelName);
    void UnloadScene();

    void LoadMesh(const std::string& meshName);
    void LoadTexture(const std::string& textureName);
};

extern std::unique_ptr<RESOURCE_SYSTEM> pResourceSystem;