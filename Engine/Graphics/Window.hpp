#pragma once
#include <string>
#include <functional>
struct GLFWwindow;

namespace gfx {
class Window {
public:
    Window(int w, int h, const char* title);
    ~Window();
    void run();               // open window and loop until closed
    void runWithCallback(std::function<void()> updateCallback); // run with custom update callback
    bool isKeyPressed(int key) const; // check if key is pressed
    bool isOpen() const;      // check if window is still open
    void close();             // close the window
    static void error_cb(int error, char const* description);
private:
    static GLFWwindow* ptr_window;
    int m_w, m_h;
    std::string m_title;
};
}