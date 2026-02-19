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
        for (auto& [id, gocPtr] : FACTORY->Objects())
        {
           

        }

    }
    /*****************************************************************************************
      \brief
         Initializes the Nav system.

      \details
         Prepares any required resources or state before the navigation system updates begin. 
         Currently logs initialization status to the console.
    *****************************************************************************************/
    void NavSystem::Initialize()
    {std::cout << "[NavSystem] Initialized.\n"; }
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