#pragma once

#include "CameraController.h"
#include "StoreScene.h"

#include <SDL.h>
#include <filament/Camera.h>
#include <filament/ColorGrading.h>
#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/SwapChain.h>
#include <filament/View.h>
#include <utils/Entity.h>

#include <filesystem>
#include <memory>
#include <string>

namespace iconmode {

struct AppConfig {
    std::filesystem::path modelPath = "assets/models/icon_mode_store.glb";
    std::string title = "ICON MODE Store - Filament Walkthrough";
    int width = 1600;
    int height = 900;
    filament::Engine::Backend backend = filament::Engine::Backend::OPENGL;
};

class StoreApp {
public:
    explicit StoreApp(AppConfig config);
    ~StoreApp();

    StoreApp(const StoreApp&) = delete;
    StoreApp& operator=(const StoreApp&) = delete;

    void run();

private:
    void initialize();
    void shutdown();
    void handleEvents();
    void resize(int width, int height);
    void renderFrame(float deltaSeconds);

    AppConfig mConfig;
    SDL_Window* mWindow = nullptr;
    void* mNativeWindow = nullptr;
    bool mRunning = true;
    int mDrawableWidth = 0;
    int mDrawableHeight = 0;

    filament::Engine* mEngine = nullptr;
    filament::Renderer* mRenderer = nullptr;
    filament::SwapChain* mSwapChain = nullptr;
    filament::Scene* mScene = nullptr;
    filament::View* mView = nullptr;
    filament::ColorGrading* mColorGrading = nullptr;
    utils::Entity mCameraEntity;
    filament::Camera* mCamera = nullptr;

    CameraController mCameraController;
    std::unique_ptr<StoreScene> mStoreScene;
};

} // namespace iconmode

