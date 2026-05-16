#include "StoreApp.h"

#include <SDL_syswm.h>
#include <filament/Viewport.h>
#include <utils/EntityManager.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace iconmode {
namespace {

void* getNativeWindow(SDL_Window* window) {
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);

    if (!SDL_GetWindowWMInfo(window, &info)) {
        throw std::runtime_error(std::string("SDL_GetWindowWMInfo failed: ") + SDL_GetError());
    }

#if defined(_WIN32)
    return info.info.win.window;
#elif defined(__APPLE__)
    return info.info.cocoa.window;
#elif defined(__linux__)
    return reinterpret_cast<void*>(info.info.x11.window);
#else
    static_assert(false, "Add native window extraction for this platform.");
#endif
}

} // namespace

StoreApp::StoreApp(AppConfig config) : mConfig(std::move(config)) {
    initialize();
}

StoreApp::~StoreApp() {
    shutdown();
}

void StoreApp::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    mWindow = SDL_CreateWindow(
        mConfig.title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        mConfig.width,
        mConfig.height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);

    if (!mWindow) {
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    mNativeWindow = getNativeWindow(mWindow);
    mEngine = filament::Engine::create(mConfig.backend);
    mRenderer = mEngine->createRenderer();
    mSwapChain = mEngine->createSwapChain(mNativeWindow);
    mScene = mEngine->createScene();
    mView = mEngine->createView();

    mCameraEntity = utils::EntityManager::get().create();
    mCamera = mEngine->createCamera(mCameraEntity);

    mView->setScene(mScene);
    mView->setCamera(mCamera);
    mView->setSampleCount(4);
    mView->setShadowingEnabled(true);

    mColorGrading = filament::ColorGrading::Builder()
        .toneMapping(filament::ColorGrading::ToneMapping::ACES)
        .exposure(0.0f)
        .build(*mEngine);
    mView->setColorGrading(mColorGrading);

    filament::Renderer::ClearOptions clearOptions;
    clearOptions.clear = true;
    clearOptions.clearColor = {0.012f, 0.011f, 0.010f, 1.0f};
    mRenderer->setClearOptions(clearOptions);

    resize(mConfig.width, mConfig.height);

    mStoreScene = std::make_unique<StoreScene>(*mEngine, *mScene, mConfig.modelPath);
    mStoreScene->load();
}

void StoreApp::shutdown() {
    mStoreScene.reset();

    if (mEngine) {
        if (mColorGrading) {
            mEngine->destroy(mColorGrading);
            mColorGrading = nullptr;
        }
        if (mCamera) {
            mEngine->destroyCameraComponent(mCameraEntity);
            mCamera = nullptr;
        }
        if (mView) {
            mEngine->destroy(mView);
            mView = nullptr;
        }
        if (mScene) {
            mEngine->destroy(mScene);
            mScene = nullptr;
        }
        if (mSwapChain) {
            mEngine->destroy(mSwapChain);
            mSwapChain = nullptr;
        }
        if (mRenderer) {
            mEngine->destroy(mRenderer);
            mRenderer = nullptr;
        }
        filament::Engine::destroy(&mEngine);
    }

    if (mWindow) {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
    }

    SDL_Quit();
}

void StoreApp::run() {
    using clock = std::chrono::steady_clock;
    auto previous = clock::now();

    while (mRunning) {
        const auto now = clock::now();
        const float deltaSeconds = std::chrono::duration<float>(now - previous).count();
        previous = now;

        handleEvents();
        renderFrame(deltaSeconds);
    }
}

void StoreApp::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            mRunning = false;
        }

        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
            mRunning = false;
        }

        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            resize(event.window.data1, event.window.data2);
        }

        mCameraController.handleEvent(event);
    }
}

void StoreApp::resize(int width, int height) {
    mDrawableWidth = width > 0 ? width : 1;
    mDrawableHeight = height > 0 ? height : 1;
    mView->setViewport({0, 0, static_cast<uint32_t>(mDrawableWidth), static_cast<uint32_t>(mDrawableHeight)});
    mCameraController.setViewport(mDrawableWidth, mDrawableHeight);
}

void StoreApp::renderFrame(float deltaSeconds) {
    mCameraController.update(*mCamera, deltaSeconds);

    if (mRenderer->beginFrame(mSwapChain)) {
        mRenderer->render(mView);
        mRenderer->endFrame();
    }
}

} // namespace iconmode
