#include "LightManager.h"

#include <filament/Engine.h>
#include <filament/LightManager.h>
#include <math/vec3.h>
#include <utils/EntityManager.h>

namespace iconmode {
namespace {

constexpr float kPi = 3.14159265358979323846f;

filament::math::float3 normalized(filament::math::float3 v) {
    return filament::math::normalize(v);
}

} // namespace

StoreLighting::StoreLighting(filament::Engine& engine, filament::Scene& scene)
    : mEngine(engine), mScene(scene) {
}

StoreLighting::~StoreLighting() {
    for (utils::Entity light : mLights) {
        mScene.remove(light);
        mEngine.destroy(light);
    }
    mLights.clear();

    if (mIndirectLight) {
        mScene.setIndirectLight(nullptr);
        mEngine.destroy(mIndirectLight);
        mIndirectLight = nullptr;
    }
}

void StoreLighting::buildReferenceLighting() {
    const filament::math::float3 lowWarmAmbient[9] = {
        {0.030f, 0.025f, 0.019f},
        {0.000f, 0.000f, 0.000f},
        {0.000f, 0.000f, 0.000f},
        {0.000f, 0.000f, 0.000f},
        {0.000f, 0.000f, 0.000f},
        {0.000f, 0.000f, 0.000f},
        {0.000f, 0.000f, 0.000f},
        {0.000f, 0.000f, 0.000f},
        {0.000f, 0.000f, 0.000f},
    };

    mIndirectLight = filament::IndirectLight::Builder()
        .irradiance(3, lowWarmAmbient)
        .intensity(450.0f)
        .build(mEngine);
    mScene.setIndirectLight(mIndirectLight);

    const filament::math::float3 warmWhite = {1.0f, 0.94f, 0.82f};
    const filament::math::float3 amber = {1.0f, 0.60f, 0.25f};
    const filament::math::float3 softCool = {0.72f, 0.80f, 1.0f};

    for (float z : {-4.3f, -2.7f, -1.1f, 0.7f, 2.5f, 4.2f}) {
        addSpot({-2.6f, 3.25f, z}, {0.15f, -1.0f, -0.05f}, warmWhite, 47000.0f, kPi / 12.0f, kPi / 7.0f);
        addSpot({2.6f, 3.25f, z}, {-0.15f, -1.0f, -0.05f}, warmWhite, 47000.0f, kPi / 12.0f, kPi / 7.0f);
    }

    addSpot({0.0f, 3.15f, -4.9f}, {0.0f, -0.8f, 0.5f}, warmWhite, 65000.0f, kPi / 14.0f, kPi / 6.0f);
    addPoint({0.0f, 1.15f, -5.25f}, amber, 5200.0f, 2.5f);
    addPoint({0.0f, 2.55f, -5.28f}, {1.0f, 0.90f, 0.78f}, 2200.0f, 2.0f);

    for (float z : {-3.8f, -2.0f, -0.2f, 1.6f, 3.4f}) {
        addPoint({-4.1f, 1.95f, z}, amber, 1550.0f, 1.55f);
        addPoint({4.1f, 1.95f, z}, amber, 1550.0f, 1.55f);
    }

    addPoint({-3.5f, 2.7f, 5.2f}, softCool, 1200.0f, 3.0f);
    addPoint({3.5f, 2.7f, 5.2f}, softCool, 1200.0f, 3.0f);
}

void StoreLighting::addSpot(const filament::math::float3& position,
                            const filament::math::float3& direction,
                            const filament::math::float3& color,
                            float intensity,
                            float innerCone,
                            float outerCone) {
    auto& em = utils::EntityManager::get();
    utils::Entity light = em.create();
    filament::LightManager::Builder(filament::LightManager::Type::SPOT)
        .position(position)
        .direction(normalized(direction))
        .color(color)
        .intensity(intensity)
        .falloff(8.0f)
        .spotLightCone(innerCone, outerCone)
        .castShadows(true)
        .build(mEngine, light);
    mScene.addEntity(light);
    mLights.push_back(light);
}

void StoreLighting::addPoint(const filament::math::float3& position,
                             const filament::math::float3& color,
                             float intensity,
                             float falloff) {
    auto& em = utils::EntityManager::get();
    utils::Entity light = em.create();
    filament::LightManager::Builder(filament::LightManager::Type::POINT)
        .position(position)
        .color(color)
        .intensity(intensity)
        .falloff(falloff)
        .build(mEngine, light);
    mScene.addEntity(light);
    mLights.push_back(light);
}

} // namespace iconmode

