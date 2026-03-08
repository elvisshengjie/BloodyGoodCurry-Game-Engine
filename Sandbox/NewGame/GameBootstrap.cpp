/*********************************************************************************************
 \file      GameBootstrap.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Implements the generated game project's bootstrap hooks.
 \details   Registers the template GlowComponent, installs its JSON save/load handlers,
            defines a simple player-discovery policy, and configures the generated
            project's default startup level.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "EngineCall.hpp"

#include "Components/GlowComponent.h"
#include "Composition/ComponentCreator.h"
#include "Systems/LogicSystem.h"
#include "Systems/RenderSystem.h"

#include <filesystem>
#include <memory>
#include <optional>

namespace
{
    /*************************************************************************************
     \brief  Find the first object named `Player` in the active factory.
     \param  logic  Active engine LogicSystem.
     \return Matching player object pointer, or nullptr if no such object exists.
    *************************************************************************************/
    Framework::GOC* FindPlayerByName(Framework::LogicSystem& logic)
    {
        auto* factory = logic.Factory();
        if (!factory)
            return nullptr;

        for (const auto& [id, obj] : factory->Objects())
        {
            (void)id;
            if (obj && obj->GetObjectName() == "Player")
                return obj.get();
        }

        return nullptr;
    }

    /*************************************************************************************
     \brief  Register JSON serialization hooks for the generated GlowComponent.
     \param  factory  Active game factory receiving component save/load handlers.
    *************************************************************************************/
    void InstallGlowJsonHandlers(Framework::GameObjectFactory& factory)
    {
        factory.SetComponentJsonHandlers(
            Framework::ComponentTypeId::CT_GlowComponent,
            [](const Framework::GameComponent& component) -> std::optional<Framework::json>
            {
                auto const& glow = static_cast<const Framework::GlowComponent&>(component);
                Framework::json points = Framework::json::array();
                for (const auto& point : glow.points)
                    points.push_back({ {"x", point.x}, {"y", point.y} });

                return Framework::json{
                    {"r", glow.r},
                    {"g", glow.g},
                    {"b", glow.b},
                    {"opacity", glow.opacity},
                    {"brightness", glow.brightness},
                    {"inner_radius", glow.innerRadius},
                    {"outer_radius", glow.outerRadius},
                    {"falloff_exponent", glow.falloffExponent},
                    {"visible", glow.visible},
                    {"points", points}
                };
            },
            [](Framework::GameComponent& component, const Framework::json& data) -> bool
            {
                auto& glow = static_cast<Framework::GlowComponent&>(component);

                auto readFloat = [&data](const char* key, float& out)
                {
                    auto it = data.find(key);
                    if (it != data.end() && it->is_number())
                        out = it->get<float>();
                };

                auto readBool = [&data](const char* key, bool& out)
                {
                    auto it = data.find(key);
                    if (it != data.end() && it->is_boolean())
                        out = it->get<bool>();
                };

                readFloat("r", glow.r);
                readFloat("g", glow.g);
                readFloat("b", glow.b);
                readFloat("opacity", glow.opacity);
                readFloat("brightness", glow.brightness);
                readFloat("inner_radius", glow.innerRadius);
                readFloat("outer_radius", glow.outerRadius);
                readFloat("falloff_exponent", glow.falloffExponent);
                readBool("visible", glow.visible);

                glow.points.clear();
                if (auto it = data.find("points"); it != data.end() && it->is_array())
                {
                    glow.points.reserve(it->size());
                    for (const auto& point : *it)
                    {
                        float x = 0.0f;
                        float y = 0.0f;
                        if (point.contains("x") && point["x"].is_number())
                            x = point["x"].get<float>();
                        if (point.contains("y") && point["y"].is_number())
                            y = point["y"].get<float>();
                        glow.points.emplace_back(x, y);
                    }
                }

                return true;
            });
    }
}

namespace mygame
{
    /*************************************************************************************
     \brief  Configure generated-project logic bootstrap behavior.
     \param  logic  Active engine LogicSystem prior to initialization.
    *************************************************************************************/
    void ConfigureGameBootstrap(Framework::LogicSystem& logic)
    {
        logic.SetFactorySetupCallback([](Framework::GameObjectFactory& factory)
        {
            factory.AddComponentCreator(
                "GlowComponent",
                std::make_unique<Framework::ComponentCreatorType<Framework::GlowComponent>>(
                    Framework::ComponentTypeId::CT_GlowComponent));
            InstallGlowJsonHandlers(factory);
        });

        logic.SetFindPlayerCallback(&FindPlayerByName);
        logic.SetStartupLevelPath(logic.ResolveDataPath("level.json"));
    }

    /*************************************************************************************
     \brief  Configure generated-project render bootstrap behavior.
     \param  render  Active engine RenderSystem prior to initialization.
     \details The generated template starts with no render-specific overrides.
    *************************************************************************************/
    void ConfigureRenderBootstrap(Framework::RenderSystem& render)
    {
        (void)render;
    }
}
