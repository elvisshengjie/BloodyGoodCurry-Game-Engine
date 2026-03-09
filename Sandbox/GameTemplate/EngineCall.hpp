/*********************************************************************************************
 \file      EngineCall.hpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Declares the generated game project's engine-facing bootstrap hooks.
 \details   Keeps per-project setup on the game side by exposing behaviour registration,
            logic bootstrap configuration, and render bootstrap configuration entry points.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

namespace Framework
{
    class LogicSystem;
    class RenderSystem;
}

namespace mygame
{
    /// \brief Register game-specific behaviour bindings with the engine logic system.
    void RegisterMyGameScripts(Framework::LogicSystem& logic);
    /// \brief Configure game-specific factory/bootstrap state for the generated project.
    void ConfigureGameBootstrap(Framework::LogicSystem& logic);
    /// \brief Configure any render-side defaults for the generated project.
    void ConfigureRenderBootstrap(Framework::RenderSystem& render);
}
