#pragma once
#include <string>

namespace gfx {
class Window {
public:
    Window(int w, int h, const char* title);
    ~Window();
    void run();               // open window and loop until closed
private:
    void* m_handle = nullptr; // GLFWwindow*, kept as void* to avoid leaking GLFW in headers
    int m_w, m_h;
    std::string m_title;
};
}