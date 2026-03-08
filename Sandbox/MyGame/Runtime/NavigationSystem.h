/*********************************************************************************************
 \file      NavigationSystem.h
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Declares the NavSystem used by BloodyGoodCurry to build waypoint graphs and
            drive path-following movement for enemies.

 \details   This runtime system is game-owned because it depends on the current game's
            waypoint and enemy decision-tree components. It scans waypoint objects to
            build a navigation graph, then advances enemy path state each frame by
            steering toward the next waypoint in the active path.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include "Factory/Factory.h"
#include "Physics/Dynamics/RigidBodyComponent.h"
#include "Component/TransformComponent.h"
#include "Graphics/Window.hpp"
#include "Components/WayPointComponent.h"
#include "Components/EnemyDecisionTreeComponent.h"
#include "Systems/AI_Navigation/GraphNode.h"
#include "Systems/AI_Navigation/NavGraph.h"
#include <vector>
#include <unordered_map>
#include <queue>
#include <cmath>
#include <algorithm>
namespace Framework
{
    class NavSystem : public Framework::ISystem {
    public:
        /*************************************************************************************
         \brief Constructs the navigation system with the active window.
         \param window Window used for optional debug rendering hooks.
        *************************************************************************************/
        explicit NavSystem(gfx::Window& window);

        /*************************************************************************************
         \brief Builds the initial waypoint graph for the current level.
        *************************************************************************************/
        void Initialize() override;

        /*************************************************************************************
         \brief Advances path-following movement for all enemies with active paths.
         \param dt Delta time in seconds.
        *************************************************************************************/
        void Update(float dt) override;

        /*************************************************************************************
         \brief Draws optional debug path/graph visualization.
        *************************************************************************************/
        void draw() override;

        /*************************************************************************************
         \brief Releases transient navigation state.
        *************************************************************************************/
        void Shutdown() override;

        /*************************************************************************************
         \brief Returns the diagnostic system name.
        *************************************************************************************/
        std::string GetName() override { return "NavSystem"; }

    private:
        gfx::Window* window{};
        NavGraph navGraph;

        /*************************************************************************************
         \brief Computes a path for an enemy toward the requested graph node.
         \param enemy      Enemy object that owns the path state.
         \param goalNodeID Destination waypoint node ID.
        *************************************************************************************/
        void RequestPath(GOC* enemy, int goalNodeID);

        /*************************************************************************************
         \brief Rebuilds the internal navigation graph from live waypoint objects.
        *************************************************************************************/
        void BuildGraphFromWaypoints();
    };
}
