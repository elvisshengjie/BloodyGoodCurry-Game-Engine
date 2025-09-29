#pragma once
// Systems/RenderSystem.hpp
#pragma once
#include "Common/System.h"
#include "Factory/Factory.h"
#include "Component/TransformComponent.h"
#include "Component/RenderComponent.h"
#include "../../Sandbox/MyGame/MathUtils.hpp"      

namespace Framework {
    class RenderSystem : public Framework::ISystem {
    public:
        void Initialize() override {
            // Simple shaders: pos -> uMVP, flat color uColor
            static const char* kVS = R"(#version 330 core
      layout(location=0) in vec2 aPos;
      uniform mat4 uMVP;
      void main(){ gl_Position = uMVP * vec4(aPos, 0.0, 1.0); })";

            static const char* kFS = R"(#version 330 core
      out vec4 FragColor;
      uniform vec4 uColor;
      void main(){ FragColor = uColor; })";

            GLuint vs = Compile(GL_VERTEX_SHADER, kVS);
            GLuint fs = Compile(GL_FRAGMENT_SHADER, kFS);
            prog_ = Link(vs, fs);
            uMVP_ = glGetUniformLocation(prog_, "uMVP");
            uColor_ = glGetUniformLocation(prog_, "uColor");

            quad_.create();

            // Set a default orthographic camera (update this on resize)
            // Pixel coords: origin at (0,0) bottom-left; change if you prefer.
            proj_ = Ortho(0.f, (float)screenW_, 0.f, (float)screenH_, -1.f, 1.f);

            glUseProgram(prog_);
            glUniform4f(uColor_, 1, 1, 1, 1);
            glUseProgram(0);
        }

        // Call this if your window resizes
        void SetViewport(int w, int h) {
            screenW_ = w; screenH_ = h;
            proj_ = Ortho(0.f, (float)w, 0.f, (float)h, -1.f, 1.f);
        }

        void Update(float dt) override {
            (void)dt;
            glUseProgram(prog_);
            glBindVertexArray(quad_.vao);

            for (const auto& [id, obj] : FACTORY->Objects()) {
                if (!obj) continue;

                auto* tr = obj->GetComponentType<Framework::TransformComponent>(
                    Framework::ComponentTypeId::CT_TransformComponent);
                auto* rc = obj->GetComponentType<Framework::RenderComponent>(
                    Framework::ComponentTypeId::CT_RenderComponent);

                if (!tr || !rc) continue;

                // Model = T * R * S  (column-major, OpenGL style)
                Mat4 T = Translate(tr->x, tr->y);
                Mat4 R = RotateZ(/* assume degrees in Transform */ DegToRad(tr->rot));
                Mat4 S = Scale(rc->w, rc->h);
                Mat4 M = Mul(Mul(T, R), S);
                Mat4 MVP = Mul(proj_, M);

                glUniformMatrix4fv(uMVP_, 1, GL_FALSE, MVP.m);
                glUniform4f(uColor_, rc->r, rc->g, rc->b, rc->a);

                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);
            }

            glBindVertexArray(0);
            glUseProgram(0);
        }

        std::string GetName() override { return "RenderSystem"; }

        ~RenderSystem() {
            if (prog_) glDeleteProgram(prog_);
            quad_.destroy();
        }

    private:
        int screenW_ = 1280, screenH_ = 720; // set at init from your Window
        Mat4 proj_{};
        GLuint prog_{ 0 };
        GLint  uMVP_{ -1 }, uColor_{ -1 };
        QuadGL quad_;
    };
}