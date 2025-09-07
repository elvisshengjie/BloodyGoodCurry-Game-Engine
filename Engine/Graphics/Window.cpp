#include "Graphics/Window.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace gfx {
Window::Window(int w, int h, const char* title) : m_w(w), m_h(h), m_title(title) {
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");
    GLFWwindow* win = glfwCreateWindow(m_w, m_h, m_title.c_str(), nullptr, nullptr);
    if (!win) { glfwTerminate(); throw std::runtime_error("glfwCreateWindow failed"); }
    glfwMakeContextCurrent(win);
    m_handle = win;
}
Window::~Window() {
    if (m_handle) {
        glfwDestroyWindow(static_cast<GLFWwindow*>(m_handle));
        glfwTerminate();
    }
}
void Window::run() {
    auto* win = static_cast<GLFWwindow*>(m_handle);
    while (!glfwWindowShouldClose(win)) {
        // no GL calls needed; just swap/poll
        glfwSwapBuffers(win);
        glfwPollEvents();
    }
}
}

