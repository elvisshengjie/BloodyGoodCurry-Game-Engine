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
#include "LogicSystem.h"

#include "Component/CircleRenderComponent.h"
#include "Component/RenderComponent.h"
#include "Component/SpriteComponent.h"
#include "Component/TransformComponent.h"
#include "Config/WindowConfig.h"
#include "Debug/CrashLogger.hpp"
#include "Debug/ImGuiLayer.h"
#include "Debug/Perf.h"
#include "Debug/Spawn.h"
#include "Factory/Factory.h"
#include "Graphics/Graphics.hpp"
#include "Graphics/Window.hpp"
#include "Resource_Manager/Resource_Manager.h"
#include "Graphics/GraphicsText.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <chrono>
#include <iostream>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif  
#include <filesystem>
#include <string>
namespace gfx {
    class Window;
}

namespace Framework {
    class LogicSystem;

    class RenderSystem : public Framework::ISystem {
    public:
        RenderSystem(gfx::Window& window, LogicSystem& logic);

        void Initialize() override;

        void Update(float dt) override { (void)dt; }

        std::string GetName() override { return "RenderSystem"; }

        void Shutdown() override;
        void draw() override;

    private:
        std::filesystem::path GetExeDir() const;
        std::string FindRoboto() const;

        unsigned CurrentPlayerTexture() const;
        int CurrentColumns() const;
        int CurrentRows() const;

        gfx::Window* window;
        LogicSystem& logic;

        int screenW = 1280;
        int screenH = 720;

        gfx::TextRenderer textTitle;
        gfx::TextRenderer textHint;
        bool textReadyTitle = false;
        bool textReadyHint = false;

        unsigned playerTex = 0;
        unsigned idleTex = 0;
        unsigned runTex = 0;
    };

};// namespace Framework