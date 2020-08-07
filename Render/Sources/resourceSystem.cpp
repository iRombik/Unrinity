#include "resourceSystem.h"
#include "shaderManager.h"
#include "meshManager.h"
#include "textureManager.h"

std::unique_ptr<RESOURCE_SYSTEM> pResourceSystem;

void RESOURCE_SYSTEM::Init()
{
    pShaderManager.reset(new SHADER_MANAGER);
    pMeshManager.reset(new MESH_MANAGER);
    pTextureManager.reset(new TEXTURE_MANAGER);

    pShaderManager->Init();
    pMeshManager->Init();
    pTextureManager->Init();

    pShaderManager->LoadShaders();
    pMeshManager->LoadCommonMeshes();
    pTextureManager->LoadCommonTextures();
}

void RESOURCE_SYSTEM::Reload()
{
    pShaderManager->ReloadShaders();
}

void RESOURCE_SYSTEM::Term()
{
    pShaderManager->TermShaders();
    pMeshManager->Term();
    pTextureManager->Term();

    pShaderManager.release();
    pMeshManager.release();
    pTextureManager.release();
}
