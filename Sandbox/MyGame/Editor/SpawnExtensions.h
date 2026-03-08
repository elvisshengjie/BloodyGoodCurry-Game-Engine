/*********************************************************************************************
 \file      SpawnExtensions.h
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Declares BloodyGoodCurry's spawn-panel extension registration entry point.
 \details   The engine owns the generic spawn panel. This file exposes the game-side hook
            that registers BloodyGoodCurry-specific component UI and apply callbacks.

 \copyright
            All content (c) 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once

#if SOFASPUDS_ENABLE_EDITOR

namespace mygame
{
    /*************************************************************************************
     \brief  Register BloodyGoodCurry's spawn-panel extension callbacks with the engine.
    *************************************************************************************/
    void RegisterMyGameSpawnPanelExtensions();
}

#endif
