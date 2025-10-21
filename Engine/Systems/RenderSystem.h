/*********************************************************************************************
 \file      RenderSystem.hpp
 \par       SofaSpuds
 \author    yimo kong (yimo.kong@digipen.edu) - Primary Author, 100%
 \brief     Simple 2D render system that draws colored quads for objects with
            TransformComponent + RenderComponent using a flat-color shader.
 \details   On Initialize(), the system compiles a minimalist GL pipeline (pos-only VS, flat FS),
            creates a unit quad (QuadGL), caches uniform locations, and builds an orthographic
            projection (origin at bottom-left). Each Update(), it iterates live objects from the
            Factory, finds Transform/Render components, builds M = T*R*S and MVP = Ortho*M, sets
            uMVP/uColor, and issues an indexed draw for the quad. Call SetViewport() on window
            resize to rebuild the projection. Intended for the sandbox/game and fits a
            component-based engine (not full ECS).
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
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
        void Initialize() override;
        // Call this if your window resizes
        //void SetViewport(int w, int h) {
        //    screenW_ = w; screenH_ = h;
        //    proj_ = Ortho(0.f, (float)w, 0.f, (float)h, -1.f, 1.f);
        //}

        void Update(float dt) override;

        std::string GetName() override { return "RenderSystem"; }

        void Shutdown() override;
        void draw()override;

    private:
        int screenW_ = 1280, screenH_ = 720; // set at init from your Window
        Mat4 proj_{};
        GLuint prog_{ 0 };
        GLint  uMVP_{ -1 }, uColor_{ -1 };
        QuadGL quad_;
    };
}