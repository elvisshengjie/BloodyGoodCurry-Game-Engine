/*********************************************************************************************
 \file      CombatAudioEvents.h
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%
 \brief     Declares shared combat-audio event types and callback signatures.
 \details   Lets engine combat systems emit generic hurt/death/hit events while the game
            layer decides how those events map to concrete sounds.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once

#include <functional>

namespace Framework
{
    class GameObjectComposition;

    enum class CombatAudioEvent
    {
        PlayerHurt,
        PlayerDeath,
        EnemyHurt,
        EnemyDeath,
        PlayerAttackHit,
        PlayerAttackBlocked,
        PlayerAttackMiss
    };

    using CombatAudioCallback = std::function<void(GameObjectComposition*, CombatAudioEvent)>;
}
