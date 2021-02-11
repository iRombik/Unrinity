#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "windowSystem.h"
#include "Components/camera.h"
#include "Events/moving.h"

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

        for (auto it = m_entityList.begin(); it != m_entityList.end(); it++) {
            bool isCameraMoved = false;
            TRANSFORM_COMPONENT* cameraPos = ECS::pEcsCoordinator->GetComponent<TRANSFORM_COMPONENT>(*it);
            ROTATE_COMPONENT* cameraRotation = ECS::pEcsCoordinator->GetComponent<ROTATE_COMPONENT>(*it);

            auto kse = GetEventList<KEY_STATE_EVENT>();
            for (auto event = kse.first; event != kse.second; event++) {
                const glm::vec3 dirLookAt = cameraRotation->quaternion * glm::vec3(1.f, 0.f, 0.f);
                const KEY_STATE_EVENT* keyboardEvent = static_cast<const KEY_STATE_EVENT*>(event->second.get());
                const KEY_STATE* keyInput = keyboardEvent->keyState;
                if (keyInput->isButtonPressed.test(GLFW_KEY_W) ||
                    keyInput->isButtonPressed.test(GLFW_KEY_A) ||
                    keyInput->isButtonPressed.test(GLFW_KEY_S) ||
                    keyInput->isButtonPressed.test(GLFW_KEY_D)) {
                    isCameraMoved = true;
                } else {
                    continue;
                }
                const glm::vec3 sidewayVector = glm::cross(UP_VECTOR, dirLookAt);

                float cameraMovementSpeed = 5.f;
                if (keyInput->isButtonWasPressed.test(GLFW_KEY_CAPS_LOCK)) {
                    cameraMovementSpeed /= 5.f;
                }
                if (keyInput->isButtonWasPressed.test(GLFW_KEY_LEFT_SHIFT)) {
                    cameraMovementSpeed *= 10.f;
                }

                if (keyInput->isButtonPressed.test(GLFW_KEY_W)) {
                    cameraPos->position += cameraMovementSpeed * dirLookAt;
                }
                if (keyInput->isButtonPressed.test(GLFW_KEY_S)) {
                    cameraPos->position -= cameraMovementSpeed * dirLookAt;
                }
                if (keyInput->isButtonPressed.test(GLFW_KEY_A)) {
                    cameraPos->position -= cameraMovementSpeed * sidewayVector;
                }
                if (keyInput->isButtonPressed.test(GLFW_KEY_D)) {
                    cameraPos->position += cameraMovementSpeed * sidewayVector;
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

                const float angleXZ = glm::radians(0.5f * mousePosShift.x);
                const float angleY = glm::radians(0.5f * mousePosShift.y);
                static float roll = 0.f;
                static float yaw = 0.f;
                yaw += angleXZ;
                roll += angleY;

                glm::quat yawQuater = glm::angleAxis(yaw, UP_VECTOR);
                glm::quat rollQuater = glm::angleAxis(-roll, glm::vec3(0.f, 0.f, 1.f));
                cameraRotation->quaternion = yawQuater * rollQuater;
            }


            if (true) {
                const glm::vec3 dirLookAt = normalize(cameraRotation->quaternion * glm::vec3(1.f, 0.f, 0.f));
                CAMERA_COMPONENT* camera = ECS::pEcsCoordinator->GetComponent<CAMERA_COMPONENT>(*it);
                camera->viewMatrix = glm::lookAt(cameraPos->position, cameraPos->position + dirLookAt, UP_VECTOR);
                camera->projMatrix = glm::perspective(camera->fov, camera->aspectRatio, camera->nearPlane, camera->farPlane);
                //https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
                const glm::mat4 clip(
                    1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, -1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.f,
                    0.0f, 0.0f, 0.0f, 1.0f
                );
                camera->viewProjMatrix = clip * camera->projMatrix * camera->viewMatrix;
            }
        }
        ClearEventList();
    }
};
