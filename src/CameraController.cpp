#include "CameraController.h"

#include <algorithm>
#include <cmath>

namespace iconmode {
namespace {

constexpr float kPi = 3.14159265358979323846f;

float radians(float degrees) {
    return degrees * kPi / 180.0f;
}

} // namespace

void CameraController::setViewport(int width, int height) {
    if (width > 0 && height > 0) {
        mAspect = static_cast<float>(width) / static_cast<float>(height);
    }
}

void CameraController::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
        mMouseLook = true;
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }

    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT) {
        mMouseLook = false;
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE && mMouseLook) {
        mMouseLook = false;
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    if (event.type == SDL_MOUSEMOTION && mMouseLook) {
        mYawDegrees += static_cast<float>(event.motion.xrel) * mMouseSensitivity;
        mPitchDegrees -= static_cast<float>(event.motion.yrel) * mMouseSensitivity;
        mPitchDegrees = std::clamp(mPitchDegrees, -75.0f, 75.0f);
    }
}

void CameraController::update(filament::Camera& camera, float deltaSeconds) {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    const float step = mMoveSpeed * deltaSeconds;
    const auto f = forward();
    const auto r = right();

    if (keys[SDL_SCANCODE_W]) {
        mPosition += f * step;
    }
    if (keys[SDL_SCANCODE_S]) {
        mPosition -= f * step;
    }
    if (keys[SDL_SCANCODE_A]) {
        mPosition -= r * step;
    }
    if (keys[SDL_SCANCODE_D]) {
        mPosition += r * step;
    }
    if (keys[SDL_SCANCODE_Q]) {
        mPosition.y -= step;
    }
    if (keys[SDL_SCANCODE_E]) {
        mPosition.y += step;
    }

    mPosition.x = std::clamp(mPosition.x, -4.2f, 4.2f);
    mPosition.y = std::clamp(mPosition.y, 0.85f, 2.35f);
    mPosition.z = std::clamp(mPosition.z, -5.1f, 6.9f);

    camera.setProjection(60.0, mAspect, 0.05, 80.0, filament::Camera::Fov::VERTICAL);
    camera.lookAt(mPosition, mPosition + f, {0.0f, 1.0f, 0.0f});
}

filament::math::float3 CameraController::forward() const {
    const float yaw = radians(mYawDegrees);
    const float pitch = radians(mPitchDegrees);
    const float cp = std::cos(pitch);
    filament::math::float3 v = {
        cp * std::cos(yaw),
        std::sin(pitch),
        cp * std::sin(yaw),
    };
    return filament::math::normalize(v);
}

filament::math::float3 CameraController::right() const {
    return filament::math::normalize(filament::math::cross(forward(), {0.0f, 1.0f, 0.0f}));
}

} // namespace iconmode

