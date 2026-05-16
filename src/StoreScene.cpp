#include "StoreScene.h"

#include <gltfio/ResourceLoader.h>
#include <gltfio/TextureProvider.h>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace iconmode {

StoreScene::StoreScene(filament::Engine& engine, filament::Scene& scene, std::filesystem::path modelPath)
    : mEngine(engine), mScene(scene), mModelPath(std::move(modelPath)) {
}

StoreScene::~StoreScene() {
    mLighting.reset();

    if (mAsset) {
        mScene.removeEntities(mAsset->getEntities(), mAsset->getEntityCount());
        mAssetLoader->destroyAsset(mAsset);
        mAsset = nullptr;
    }

    if (mAssetLoader) {
        filament::gltfio::AssetLoader::destroy(&mAssetLoader);
    }

    if (mMaterialProvider) {
        mMaterialProvider->destroyMaterials();
        delete mMaterialProvider;
        mMaterialProvider = nullptr;
    }
}

void StoreScene::load() {
    if (!std::filesystem::exists(mModelPath)) {
        std::ostringstream message;
        message << "Missing GLB model: " << mModelPath.string()
                << ". Generate it with blender --background --python blender/icon_mode_scene.py -- --skip-render";
        throw std::runtime_error(message.str());
    }

    mModelBytes = readFile(mModelPath);
    mMaterialProvider = filament::gltfio::createJitShaderProvider(&mEngine);
    mAssetLoader = filament::gltfio::AssetLoader::create({&mEngine, mMaterialProvider});

    mAsset = mAssetLoader->createAsset(mModelBytes.data(), static_cast<uint32_t>(mModelBytes.size()));
    if (!mAsset) {
        throw std::runtime_error("Filament could not parse the store GLB.");
    }

    auto* stbProvider = filament::gltfio::createStbProvider(&mEngine);
    auto* ktx2Provider = filament::gltfio::createKtx2Provider(&mEngine);
    bool resourcesLoaded = false;

    {
        std::string gltfPath = mModelPath.string();
        filament::gltfio::ResourceLoader resourceLoader({&mEngine, gltfPath.c_str(), true});
        resourceLoader.addTextureProvider("image/jpeg", stbProvider);
        resourceLoader.addTextureProvider("image/png", stbProvider);
        resourceLoader.addTextureProvider("image/ktx2", ktx2Provider);
        resourcesLoaded = resourceLoader.loadResources(mAsset);
    }

    if (!resourcesLoaded) {
        delete ktx2Provider;
        delete stbProvider;
        throw std::runtime_error("Filament failed while uploading GLB resources.");
    }

    mAsset->releaseSourceData();
    mScene.addEntities(mAsset->getEntities(), mAsset->getEntityCount());

    delete ktx2Provider;
    delete stbProvider;

    mLighting = std::make_unique<StoreLighting>(mEngine, mScene);
    mLighting->buildReferenceLighting();
}

std::vector<uint8_t> StoreScene::readFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        throw std::runtime_error("Could not open file: " + path.string());
    }

    const auto size = input.tellg();
    if (size <= 0) {
        throw std::runtime_error("File is empty: " + path.string());
    }

    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    input.seekg(0);
    input.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
}

} // namespace iconmode
