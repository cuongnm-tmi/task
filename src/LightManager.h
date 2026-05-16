#pragma once

#include <filament/IndirectLight.h>
#include <filament/Scene.h>
#include <math/vec3.h>
#include <utils/Entity.h>

#include <vector>

namespace filament {
class Engine;
}

namespace iconmode {

class StoreLighting {
public:
    StoreLighting(filament::Engine& engine, filament::Scene& scene);
    ~StoreLighting();

    StoreLighting(const StoreLighting&) = delete;
    StoreLighting& operator=(const StoreLighting&) = delete;

    void buildReferenceLighting();

private:
    void addSpot(const filament::math::float3& position,
                 const filament::math::float3& direction,
                 const filament::math::float3& color,
                 float intensity,
                 float innerCone,
                 float outerCone);
    void addPoint(const filament::math::float3& position,
                  const filament::math::float3& color,
                  float intensity,
                  float falloff);

    filament::Engine& mEngine;
    filament::Scene& mScene;
    filament::IndirectLight* mIndirectLight = nullptr;
    std::vector<utils::Entity> mLights;
};

} // namespace iconmode
