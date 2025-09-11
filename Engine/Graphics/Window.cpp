#include "Graphics/Window.hpp"

// manual GLAD uses <glad/glad.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#define GL_MINOR 5
#define GL_MAJOR 4

GLFWwindow* gfx::Window::ptr_window = nullptr;
namespace gfx {

    Window::Window(int w, int h, const char* title)
        : m_w(w), m_h(h), m_title(title)
    {
        if (!glfwInit()) {
            std::cout << "GLFW init has failed - abort program" << std::endl;

        }

        // In case GLFW fail, an error is reported to callback function
        glfwSetErrorCallback(Window::error_cb);

        // specify OpenGL version 4.5
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_MAJOR);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_MINOR);
        // MODERN OpenGL
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        //application will be double buffered
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
        // default behavior: colorbuffer is 32-bit RGBA, depthbuffer is 24-bits
        // don't change size of window
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        //Size of Viewporrt: width x height
        Window::ptr_window = glfwCreateWindow(m_w, m_h, m_title.c_str(), NULL, NULL);
        if (!Window::ptr_window) {
            std::cerr << "GLFW unable to create OpenGL context - abort program\n";
            glfwTerminate();
        
        }
        // make the previously created OpenGL context current
        glfwMakeContextCurrent(Window::ptr_window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { 
            glfwDestroyWindow(Window::ptr_window); glfwTerminate(); 
            throw std::runtime_error("Failed to initialize GLAD"); 
        }
        glViewport(0, 0, m_w, m_h);
        //Enable Vsync
        glfwSwapInterval(1);

  
    }

    Window::~Window() {
        if (Window::ptr_window) {
            glfwDestroyWindow(Window::ptr_window);
            glfwTerminate();
        }
    }

    void Window::run() {
        while (!glfwWindowShouldClose(Window::ptr_window)) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glfwSwapBuffers(Window::ptr_window);
            glfwPollEvents();
            if (glfwGetKey(Window::ptr_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(Window::ptr_window, 1);
            }
        }
    }

    void Window::error_cb(int error, char const* description)
    {
        std::cerr << "GLFW error: " << description << std::endl;
    }

} // namespace gfx
