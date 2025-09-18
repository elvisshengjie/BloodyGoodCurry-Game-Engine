#pragma once
#include "Graphics/Window.hpp"
#include "Managers/SoundManager.h"
// 这里不再需要 WindowConfig；读取配置放到 main 里

namespace mygame {
    // 由 Core 调用的四个回调
    void init(gfx::Window& win);
    void update(float dt);
    void draw();
    void shutdown();

    // 如果别处要用到，也保留音频初始化接口（内部由 init/shutdown 调）
    void initializeAudio();
    void cleanupAudio();
}
