#include <glm/gtc/random.hpp>
#include <glm/gtc/constants.hpp>

#include "game.h"
#include "commonRenderVariables.h"
#include "render.h"
#include "gui.h"
#include "terrain.h"

// #include "textureManager.h"
// #include "materialManager.h"
#include "resourceSystem.h"
#include "gameCameraSystem.h"
#include "windowSystem.h"

#include "Components/lightSource.h"
#include "Components/rendered.h"

std::unique_ptr<GAME_MANAGER> pGameManager;

bool GAME_MANAGER::Init()
{
    srand((UINT)time(0));
    ECS::pEcsCoordinator.reset(new ECS::ECS_COORDINATOR());
    pWindowSystem.reset(new WINDOW_SYSTEM());
    
    if (!pWindowSystem->Init()) {
        return false;
    }

    pResourceSystem.reset(new RESOURCE_SYSTEM());
    pResourceSystem->Init();

    RENDER_SYSTEM* renderSystem = ECS::pEcsCoordinator->CreateSystem<RENDER_SYSTEM>();
    if (!renderSystem->Init()) {
        return false;
    }

    GUI_SYSTEM* guiSystem = ECS::pEcsCoordinator->CreateSystem<GUI_SYSTEM>();
    if (!guiSystem->Init()) {
        return false;
    }

    ECS::pEcsCoordinator->CreateSystem<TERRAIN_SYSTEM>();
    return true;
}

glm::fquat GegerateRandomQuaternion() {
    glm::vec3 randAngles(glm::linearRand(0.f, 2.f * glm::pi<float>()), glm::linearRand(0.f, 2.f * glm::pi<float>()), glm::linearRand(0.f, 2.f * glm::pi<float>()));
    return glm::quat(randAngles);
}

void GAME_MANAGER::GenerateSimpleLevel()
{
    gameCamera = ECS::pEcsCoordinator->CreateEntity();
    CAMERA_COMPONENT camera;
    camera.fov = 120.f;
    camera.aspectRatio = 16.f / 9.f;
    camera.viewProjMatrix = {
        {1.f, 0.f, 0.f, 0.f},
        {0.f, 1.f, 0.f, 0.f},
        {0.f, 0.f, 1.f, 0.f},
        {0.f, 0.f, 0.f, 1.f}
    };
    ECS::pEcsCoordinator->AddComponentToEntity(gameCamera, camera);
    ECS::pEcsCoordinator->AddComponentToEntity(gameCamera, TRANSFORM_COMPONENT(glm::vec3(-35.f, 0.f, 0.f)));
    ECS::pEcsCoordinator->AddComponentToEntity(gameCamera, ROTATE_COMPONENT());
    ECS::pEcsCoordinator->AddComponentToEntity(gameCamera, INPUT_CONTROLLED());

    GAME_CAMERA_CONROL* pCameraSystem = ECS::pEcsCoordinator->CreateSystem<GAME_CAMERA_CONROL>();
    pCameraSystem->Init();

    for (int i = 0; i < 300; i++) {
        ECS::ENTITY_TYPE cubeEntity = ECS::pEcsCoordinator->CreateEntity();
        TRANSFORM_COMPONENT cubeTransform;
        cubeTransform.position = glm::ballRand(30.f);
        ROTATE_COMPONENT cubeRotate;
        cubeRotate.quaternion = GegerateRandomQuaternion();

        ECS::pEcsCoordinator->AddComponentToEntity(cubeEntity, cubeTransform);
        ECS::pEcsCoordinator->AddComponentToEntity(cubeEntity, cubeRotate);
        ECS::pEcsCoordinator->AddComponentToEntity(cubeEntity, RENDERED_COMPONENT());
    }

    const glm::vec2 terrainStartPos = { -32.f, -32.f };
    const glm::vec2 terrainSize = { 64.f, 64.f };
    ECS::pEcsCoordinator->GetSystem<TERRAIN_SYSTEM>()->Init(terrainStartPos, terrainSize);
}

void GAME_MANAGER::GeneratePBRTestLevel() {
/*
    const glm::vec3 center = { 0.f, 20.f, 0.f };

    gameCamera = ECS::pEcsCoordinator->CreateEntity();
    CAMERA_COMPONENT camera;
    camera.fov = 120.f;
    camera.aspectRatio = 16.f / 9.f;
    camera.nearPlane = 1.f;
    camera.farPlane = 256.f;
    camera.viewProjMatrix = {
        {1.f, 0.f, 0.f, 0.f},
        {0.f, 1.f, 0.f, 0.f},
        {0.f, 0.f, 1.f, 0.f},
        {0.f, 0.f, 0.f, 1.f}
    };
    ECS::pEcsCoordinator->AddComponentToEntity(gameCamera, std::move(camera));
    ECS::pEcsCoordinator->AddComponentToEntity(gameCamera, TRANSFORM_COMPONENT(center + glm::vec3(-35.f, 0.f, 0.f)));
    ECS::pEcsCoordinator->AddComponentToEntity(gameCamera, ROTATE_COMPONENT());
    ECS::pEcsCoordinator->AddComponentToEntity(gameCamera, INPUT_CONTROLLED());

    GAME_CAMERA_CONROL* pCameraSystem = ECS::pEcsCoordinator->CreateSystem<GAME_CAMERA_CONROL>();
    pCameraSystem->Init();

    MESH_COMPONENT mesh;
    mesh.pMesh = pMeshManager->GetMeshByName("sphere");

    MATERIAL_COMPONENT metalMaterial;
    metalMaterial.pAlbedoTex = pTextureManager->GetTexture("Metal002/Metal002_2K_Color.jpg");
    metalMaterial.pNormalTex = pTextureManager->GetTexture("Metal002/Metal002_2K_Normal.jpg");
    metalMaterial.pDisplacementTex = pTextureManager->GetTexture("Metal002/Metal002_2K_Displacement.jpg");
    metalMaterial.pRoughnessTex = pTextureManager->GetTexture("Metal002/Metal002_2K_Roughness.jpg");
    metalMaterial.pMetalnessTex = pTextureManager->GetTexture("Metal002/Metal002_2K_Metalness.jpg");

    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            ECS::ENTITY_TYPE cubeEntity = ECS::pEcsCoordinator->CreateEntity();
            TRANSFORM_COMPONENT cubeTransform = glm::vec3(0.f, -8.f + 3.f * j, -8.f + 3.f * i) + center;
            ROTATE_COMPONENT rotation = glm::fquat();
            ECS::pEcsCoordinator->AddComponentToEntity(cubeEntity, cubeTransform);
            ECS::pEcsCoordinator->AddComponentToEntity(cubeEntity, metalMaterial);

            CUSTOM_MATERIAL_COMPONENT customMaterialParams;
            customMaterialParams.customRoughness = 0.2f * j;
            customMaterialParams.customMetalness = glm::vec3(0.2f * i);
            ECS::pEcsCoordinator->AddComponentToEntity(cubeEntity, customMaterialParams);
            ECS::pEcsCoordinator->AddComponentToEntity(cubeEntity, mesh);

            ECS::pEcsCoordinator->AddComponentToEntity(cubeEntity, RENDERED_COMPONENT());
        }
    }

    {
        ECS::ENTITY_TYPE lightEntity = ECS::pEcsCoordinator->CreateEntity();
        POINT_LIGHT_COMPONENT light;
        light.color = glm::vec3(1.f);
        light.areaLight = 100.0f;
        light.intensity = 3.f;
        ECS::pEcsCoordinator->AddComponentToEntity(lightEntity, light);

        TRANSFORM_COMPONENT transform;
        transform.position = center + glm::vec3(-30.f, 0.f, 0.f);
        ECS::pEcsCoordinator->AddComponentToEntity(lightEntity, transform);

        CAMERA_COMPONENT lightCamera;
        lightCamera.aspectRatio = 1.f;
        lightCamera.nearPlane = 0.1;
        lightCamera.farPlane = 100.f;
        lightCamera.fov = 140.f;
        ECS::pEcsCoordinator->AddComponentToEntity(lightEntity, lightCamera);

        pointLights.push_back(lightEntity);
    }

    {
        directionalLight = ECS::pEcsCoordinator->CreateEntity();
        DIRECTIONAL_LIGHT_COMPONENT light;
        light.color = glm::vec3(1.f);
        light.attenuationParam = 0.005f;
        light.intensity = 3.f;
        ECS::pEcsCoordinator->AddComponentToEntity(directionalLight, light);

        TRANSFORM_COMPONENT transform;
        transform.position = center + glm::vec3(15.f, 30.f, 0.f);
        ECS::pEcsCoordinator->AddComponentToEntity(directionalLight, transform);

        CAMERA_COMPONENT lightCamera;
        lightCamera.aspectRatio = 1.f;
        lightCamera.nearPlane = 0.1;
        lightCamera.farPlane = 100.f;
        lightCamera.fov = 140.f;
        ECS::pEcsCoordinator->AddComponentToEntity(directionalLight, lightCamera);
    }

    const glm::vec2 terrainStartPos = { -128.f, -128.f };
    const glm::vec2 terrainSize = { 256.f, 256.f };
    ECS::pEcsCoordinator->GetSystem<TERRAIN_SYSTEM>()->Init(terrainStartPos, terrainSize);*/
}

void GAME_MANAGER::LoadSponzaLevel()
{
    pResourceSystem->LoadScene("Sponza");
}

void GAME_MANAGER::Run() {
    LoadSponzaLevel();

    while (true)
    {
        bool isWindowAlive = pWindowSystem->Update();
        if (!isWindowAlive) {
            break;
        }
        //Updates
        ECS::pEcsCoordinator->GetSystem<GAME_CAMERA_CONROL>()->Update();
        ECS::pEcsCoordinator->GetSystem<GUI_SYSTEM>()->Update();
        ECS::pEcsCoordinator->GetSystem<RENDER_SYSTEM>()->Update();

        //Render
        ECS::pEcsCoordinator->GetSystem<RENDER_SYSTEM>()->Render();
    }
}

void GAME_MANAGER::Term()
{
    ECS::pEcsCoordinator->GetSystem<GUI_SYSTEM> ()->Term();
    ECS::pEcsCoordinator->GetSystem<RENDER_SYSTEM>()->Term();
    pWindowSystem->Term();
}
