#pragma once

#include <SDL.h>
#include <filament/Camera.h>
#include <math/vec3.h>

namespace iconmode {

class CameraController {
public:
    void setViewport(int width, int height);
    void handleEvent(const SDL_Event& event);
    void update(filament::Camera& camera, float deltaSeconds);

private:
    filament::math::float3 forward() const;
    filament::math::float3 right() const;

    filament::math::float3 mPosition = {0.0f, 1.55f, 6.8f};
    float mYawDegrees = -90.0f;
    float mPitchDegrees = -3.0f;
    float mMoveSpeed = 3.2f;
    float mMouseSensitivity = 0.09f;
    float mAspect = 16.0f / 9.0f;
    bool mMouseLook = false;
};

} // namespace iconmode

