#pragma once
#include <string>
struct GLFWwindow;

namespace gfx {
class Window {
public:
    Window(int w, int h, const char* title);
    ~Window();
    void run();               // open window and loop until closed
    static void error_cb(int error, char const* description);
private:
    static GLFWwindow* ptr_window;
    int m_w, m_h;
    std::string m_title;
};
}