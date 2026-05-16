#pragma once

#include "LightManager.h"

#include <filament/Engine.h>
#include <filament/Scene.h>
#include <gltfio/AssetLoader.h>
#include <gltfio/FilamentAsset.h>
#include <gltfio/MaterialProvider.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace iconmode {

class StoreScene {
public:
    StoreScene(filament::Engine& engine, filament::Scene& scene, std::filesystem::path modelPath);
    ~StoreScene();

    StoreScene(const StoreScene&) = delete;
    StoreScene& operator=(const StoreScene&) = delete;

    void load();

private:
    static std::vector<uint8_t> readFile(const std::filesystem::path& path);

    filament::Engine& mEngine;
    filament::Scene& mScene;
    std::filesystem::path mModelPath;

    std::vector<uint8_t> mModelBytes;
    filament::gltfio::MaterialProvider* mMaterialProvider = nullptr;
    filament::gltfio::AssetLoader* mAssetLoader = nullptr;
    filament::gltfio::FilamentAsset* mAsset = nullptr;
    std::unique_ptr<StoreLighting> mLighting;
};

} // namespace iconmode

