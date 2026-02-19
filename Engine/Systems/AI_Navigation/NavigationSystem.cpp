/*********************************************************************************************
 \file      NavigationSystem.cpp
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Implementation of the NavigationSystem class, which manages AI navigation
            and pathfinding for entities.

 \details
            NavigationSystem is responsible for computing and updating navigation paths
            for entities each frame using the A* (A-Star) pathfinding algorithm.

            The system evaluates navigation requests, generates optimal paths based on
            heuristic cost evaluation, and guides entities along their computed paths
            while avoiding obstacles.

            It interacts with the Factory to access relevant game objects and
            navigation components (e.g., NavAgentComponent), and may interface with
            grid or graph-based map representations.

            The system also provides initialization, optional debug visualization
            of paths and nodes, and shutdown functionality.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "NavigationSystem.h"
#include <iostream>
#include "Common/CRTDebug.h"   // <- bring in DBG_NEW

#ifdef _DEBUG
#define new DBG_NEW       // <- redefine new AFTER all includes
#endif
namespace Framework
{
    /*****************************************************************************************
     \brief
        Constructs the NavigationSystem with a reference to the main window.
     \param window
        Reference to the game's graphics window, used for optional debug rendering.
    *****************************************************************************************/
    NavSystem::NavSystem(gfx::Window& window): window(&window){}

    void NavSystem::BuildGraphFromWaypoints()
    {
        navGraph.Clear();
        //PASS 1 - For Nodes
        for (auto& [id, gocPtr] : FACTORY->Objects())
        {
            if (!gocPtr) continue;

            auto* waypoint = gocPtr->GetComponentType<WayPointComponent>(
                ComponentTypeId::CT_WayPointComponent);
            if (!waypoint) continue;
            auto* transform = gocPtr->GetComponentType
                <TransformComponent>(ComponentTypeId::CT_TransformComponent);
            if (!transform) continue;

            navGraph.AddNode(id, transform->x, transform->y);
        }
        //PASS 2 - Add Edges
        for (auto& [id, gocPtr] : FACTORY->Objects())
        {
            if (!gocPtr) continue;
            auto* waypoint = gocPtr->GetComponentType<WayPointComponent>(
                ComponentTypeId::CT_WayPointComponent);
            if (!waypoint) continue;
            for (int neighborID : waypoint->NeighborIDs)
                navGraph.AddDirectedEdge(id, neighborID);
        }
    }
    void NavSystem::RequestPath(GOC* enemy, int goalNodeID)
    {
        if (!enemy) return;
        auto* transform = enemy->GetComponentType
        <TransformComponent>(ComponentTypeId::CT_TransformComponent);
        auto* ai = enemy->GetComponentType
            <EnemyDecisionTreeComponent>(ComponentTypeId::CT_EnemyDecisionTreeComponent);
        if (!transform || !ai) return;
        int startID = navGraph.FindNearestNode(transform->x, transform->y);
        ai->currentPathNodeIDs = navGraph.FindPath(startID, goalNodeID);
        ai->currentPathIndex = 0;
    }
    /*****************************************************************************************
      \brief
         Initializes the Nav system.

      \details
         Prepares any required resources or state before the navigation system updates begin. 
         Currently logs initialization status to the console.
    *****************************************************************************************/
    void NavSystem::Initialize()
    {
        BuildGraphFromWaypoints();
        std::cout << "[NavSystem] Initialized.\n"; 
    }
    /*****************************************************************************************
     \brief
        Updates all AI-controlled entities.

     \param dt
        Delta time (time elapsed since the last frame), used for time-based updates.

     \details
        Iterates through all game objects retrieved from the Factory. For each object,
        it attempts to run the default enemy decision tree safely using the provided delta time.
    *****************************************************************************************/
    void NavSystem::Update(float dt)
    {
        for (auto& [id, gocPtr] : FACTORY->Objects())
        {
            if (!gocPtr) continue;
            auto* ai = gocPtr->GetComponentType<EnemyDecisionTreeComponent>(
                ComponentTypeId::CT_EnemyDecisionTreeComponent);
            if (!ai || !ai->HasPath()) continue;
            auto* transform = gocPtr->GetComponentType
                <TransformComponent>(ComponentTypeId::CT_TransformComponent);
            auto* rb = gocPtr->GetComponentType<RigidBodyComponent>(
                ComponentTypeId::CT_RigidBodyComponent); 
            if (!transform) continue;
            int nodeID = ai->GetCurrentNodeID();
            const GraphNode* node = navGraph.GetNode(nodeID);
            if (!node) continue;
            float dx = node->x - transform->x;
            float dy = node->y - transform->y;
            float distance = std::sqrt(dx * dx + dy * dy);
            const float reachThreshold = 0.2f;
            if (distance < reachThreshold)
            { 
                ai->AdvancePath();
                if (rb)
                {
                    rb->velX = 0.0f;
                    rb->velY = 0.0f;
                }
                continue;
            }
            else
            {
                float invLength = 1.0f / distance;
                float dirX = dx * invLength;
                float dirY = dy * invLength;
                float speed = (ai->chaseSpeed > 0.0f) ? ai->chaseSpeed : 10.0f;
                if (rb)
                {
                    rb->velX = dirX * speed;
                    rb->velY = dirY * speed;
                }
                else
                {
                    transform->x += dirX * speed * dt;
                    transform->y += dirY * speed * dt;
                }

            }
        }
    }

    /*****************************************************************************************
     \brief
        Optional debug drawing for AI visualization.

     \details
        Currently empty. Can be extended to render AI debug information, such as
        decision tree states or enemy paths.
    *****************************************************************************************/
    void NavSystem::draw()
    {
#ifdef _DEBUG
        for (int id : navGraph.GetAllNodesIDs())
        {
            const GraphNode* node = navGraph.GetNode(id);
            if (!node) continue;
            for (int neighborID : node->NeighborIDs)
            {
                const GraphNode* neighbor = navGraph.GetNode(neighborID);
                if (!neighbor) continue;
                /*     window->drawLine(
                    node->x, node->y,
                    neighbor->x, neighbor->y,
                    gfx::Color(0, 255, 0)
                );*/
            }
        }
#endif
    }
    /*****************************************************************************************
     \brief
        Shuts down the AI system.

     \details
        Cleans up AI-related resources and logs shutdown status to the console.
    *****************************************************************************************/
    void NavSystem::Shutdown()
    {
        std::cout << "[NavSystem] Shutdown.\n";
    }
}