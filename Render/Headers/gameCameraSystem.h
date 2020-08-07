#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "windowSystem.h"
#include "Components/camera.h"
#include "Events/camera.h"
#include "Events/moving.h"

class CAMERA_MATRIX_UPDATE_SYSTEM : public ECS::SYSTEM<CAMERA_MATRIX_UPDATE_SYSTEM> {
public:
    void Init() {
        ECS::pEcsCoordinator->SubscrubeSystemToComponentType<CAMERA_TRANSFORM_COMPONENT>(this);
        ECS::pEcsCoordinator->SubscrubeSystemToComponentType<CAMERA_COMPONENT>(this);

        ECS::pEcsCoordinator->SubscrubeSystemToEventType<UPDATE_CAMERA_MATRIX_EVENT>(this);
    }

    void Update() {
        if (!IsEventList<UPDATE_CAMERA_MATRIX_EVENT>()) {
            return;
        }
        const auto& updateMatrixEvent = GetEventList<UPDATE_CAMERA_MATRIX_EVENT>();
        const UPDATE_CAMERA_MATRIX_EVENT* cameraMatrixEvent = static_cast<const UPDATE_CAMERA_MATRIX_EVENT*>(updateMatrixEvent.first->second.get());
        const CAMERA_TRANSFORM_COMPONENT* cameraPos = ECS::pEcsCoordinator->GetComponent<CAMERA_TRANSFORM_COMPONENT>(cameraMatrixEvent->entityId);
        const glm::mat4 viewMatrix = glm::lookAt(cameraPos->position, cameraPos->position + cameraPos->dirLookAt, glm::vec3(0.f, 1.f, 0.f));

        CAMERA_COMPONENT* camera = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(cameraMatrixEvent->entityId);
        const glm::mat4 perspectiveMat = glm::perspective(camera->fov, camera->aspectRatio, 1.f, 100.f);
        //https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
        const glm::mat4 clip(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
        camera->viewProjMatrix = clip * perspectiveMat * viewMatrix;
        ClearEventList();
    }
};

class GAME_CAMERA_CONROL : public ECS::SYSTEM<GAME_CAMERA_CONROL> {
public:
    void Init() {
        ECS::pEcsCoordinator->SubscrubeSystemToComponentType<CAMERA_COMPONENT>(this);
        ECS::pEcsCoordinator->SubscrubeSystemToComponentType<INPUT_CONTROLLED>(this);

        ECS::pEcsCoordinator->SubscrubeSystemToEventType<KEY_STATE_EVENT>(this);
        ECS::pEcsCoordinator->SubscrubeSystemToEventType<MOUSE_STATE_EVENT>(this);
    }

    void Update() {
        if(m_eventList.empty()) {
            return;
        }
        const glm::vec3 upVector(0.f, 1.f, 0.f);

        for (auto it = m_entityList.begin(); it != m_entityList.end(); it++) {
            bool isCameraMoved = false;
            CAMERA_TRANSFORM_COMPONENT* cameraPos = ECS::pEcsCoordinator->GetComponent<CAMERA_TRANSFORM_COMPONENT>(*it);

            auto kse = GetEventList<KEY_STATE_EVENT>();
            for (auto event = kse.first; event != kse.second; event++) {
                const KEY_STATE_EVENT* keyboardEvent = static_cast<const KEY_STATE_EVENT*>(event->second.get());
                const SHORT_KEY_STATE& keyInput = keyboardEvent->keyState;
                if (keyInput.isButtonPressed.any()) {
                    isCameraMoved = true;
                } else {
                    continue;
                }
                const glm::vec3 forwardVector = cameraPos->dirLookAt;
                const glm::vec3 sidewayVector = glm::cross(upVector, forwardVector);

                if (keyInput.isButtonPressed[0]) {
                    cameraPos->position += 0.1f * forwardVector;
                }
                if (keyInput.isButtonPressed[2]) {
                    cameraPos->position -= 0.1f * forwardVector;
                }
                if (keyInput.isButtonPressed[1]) {
                    cameraPos->position -= 0.1f * sidewayVector;
                }
                if (keyInput.isButtonPressed[3]) {
                    cameraPos->position += 0.1f * sidewayVector;
                }
            }

            auto mse = GetEventList<MOUSE_STATE_EVENT>();
            for (auto event = mse.first; event != mse.second; event++) {
                const MOUSE_STATE_EVENT* mouseEvent = static_cast<const MOUSE_STATE_EVENT*>(event->second.get());
                const MOUSE_POSITION& mousePosShift = mouseEvent->mouseShift;
                if (mousePosShift.x || mousePosShift.y) {
                    isCameraMoved = true;
                } else {
                    continue;
                }

                const float angleXZ = 0.5f * mousePosShift.x / 180.f * 3.14f;
                const float angleY = 0.2f * mousePosShift.y / 180.f * 3.14f;
                glm::vec3 result = glm::rotate(cameraPos->dirLookAt, angleXZ, upVector);
                glm::vec3 normal = glm::cross(upVector, result);
                result = glm::rotate(result, angleY, normal);
                cameraPos->dirLookAt = glm::normalize(result);
            }


            if (isCameraMoved) {
                ECS::pEcsCoordinator->SendEvent<UPDATE_CAMERA_MATRIX_EVENT>(UPDATE_CAMERA_MATRIX_EVENT(*it));
            }
        }
        ClearEventList();
    }
};
