/*********************************************************************************************
 \file      ZoomTriggerSystem.h
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Declares the game-owned ZoomTriggerSystem used to apply camera zoom changes
            when the player overlaps BloodyGoodCurry zoom trigger objects.

 \details   This system remains on the game side because it depends on the current game's
            PlayerComponent and ZoomTriggerComponent definitions. It runs after physics so
            trigger evaluation uses the latest player position from the frame.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once

#include "Common/System.h"

namespace Framework
{
    class ZoomTriggerSystem : public ISystem
    {
    public:
        /*************************************************************************************
         \brief No-op initialization hook kept for SystemManager symmetry.
        *************************************************************************************/
        void Initialize() override {}

        /*************************************************************************************
         \brief Checks player-vs-trigger overlap and applies trigger camera zoom.
         \param dt Delta time in seconds (unused).
        *************************************************************************************/
        void Update(float dt) override;

        /*************************************************************************************
         \brief No-op shutdown hook kept for SystemManager symmetry.
        *************************************************************************************/
        void Shutdown() override {}

        /*************************************************************************************
         \brief Returns the diagnostic system name.
        *************************************************************************************/
        std::string GetName() override { return "ZoomTriggerSystem"; }
    };
}
