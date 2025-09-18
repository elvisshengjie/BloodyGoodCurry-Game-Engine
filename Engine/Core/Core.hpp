#pragma once
#include <memory>
#include <chrono>
#include "Graphics/Window.hpp"

class Core {
public:
    using InitFn = void(*)(gfx::Window&);
    using UpdateFn = void(*)(float);
    using RenderFn = void(*)();
    using ShutdownFn = void(*)();

    Core(int width, int height, const char* title);
    ~Core() = default;

    // 只保留这一个主循环
    void Run();

    // 可在游戏内请求退出
    void Quit();

    // 将游戏层的回调挂进来
    void SetCallbacks(InitFn i, UpdateFn u, RenderFn r, ShutdownFn s) {
        init = i; update = u; render = r; shutdown = s;
    }

private:
    using Clock = std::chrono::steady_clock;
    using SecondsF = std::chrono::duration<float>;

    bool m_Running{ false };
    std::unique_ptr<gfx::Window> m_Window;

    // 回调
    InitFn     init{ nullptr };
    UpdateFn   update{ nullptr };
    RenderFn   render{ nullptr };
    ShutdownFn shutdown{ nullptr };
};
