#include "Graphics/Window.hpp"

// manual GLAD uses <glad/glad.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <cstdio>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

// ImGui
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

static void stb_smoke_test(const char* path) {
    int w = 0, h = 0, comp = 0;
    stbi_set_flip_vertically_on_load(0);
    unsigned char* pixels = stbi_load(path, &w, &h, &comp, 4); // force RGBA
    if (!pixels) {
        std::printf("stb_image: FAILED to load '%s' (%s)\n",
            path, stbi_failure_reason());
        return;
    }
    std::printf("stb_image: loaded '%s' %dx%d comp=%d (RGBA provided)\n",
        path, w, h, comp);
    stbi_image_free(pixels);
}

static void glm_smoke_test() {
    glm::vec3 t(1.0f, 2.0f, 3.0f);
    glm::mat4 m(1.0f);                  // identity matrix
    m = glm::translate(m, t);           // translate by (1,2,3)
    glm::vec4 p = m * glm::vec4(0, 0, 0, 1);  // apply transform to origin
    std::printf("GLM: translated origin = (%f, %f, %f)\n", p.x, p.y, p.z);
}

namespace gfx {

    Window::Window(int w, int h, const char* title)
        : m_w(w), m_h(h), m_title(title)
    {
        if (!glfwInit()) throw std::runtime_error("glfwInit failed");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* win = glfwCreateWindow(m_w, m_h, m_title.c_str(), nullptr, nullptr);
        if (!win) { glfwTerminate(); throw std::runtime_error("glfwCreateWindow failed"); }
        glfwMakeContextCurrent(win);

        // manual GLAD loader signature:
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            glfwDestroyWindow(win);
            glfwTerminate();
            throw std::runtime_error("Failed to initialize GLAD");
        }
        glm_smoke_test();
        stb_smoke_test("../../assets/check.png");
        std::printf("GL_VERSION: %s\n", glGetString(GL_VERSION));

        glViewport(0, 0, m_w, m_h);
        glfwSwapInterval(1);

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
            glClearColor(0.1f, 0.2f, 0.35f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glfwSwapBuffers(win);
            glfwPollEvents();
            if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(win, 1);
        }
    }

} // namespace gfx
