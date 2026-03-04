/*********************************************************************************************
 \file      PlayerHUD.h
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%
 \brief     Declares the game-specific PlayerHUDComponent.
 \details   BloodyGoodCurry owns this component declaration and implementation. The engine
            only keeps the shared component type ID so prefab/level data can still refer
            to the HUD component without baking its behavior into the engine target.
*********************************************************************************************/
#pragma once

#include "Composition/Component.h"
#include "Memory/ComponentPool.h"
#include "Graphics/Graphics.hpp"
#include <array>

namespace Framework
{
    class PlayerHealthComponent;

    class PlayerHUDComponent : public GameComponent
    {
    public:
        void initialize() override;
        void SendMessage(Message& m) override;
        void Serialize(ISerializer& s) override;
        ComponentHandle Clone() const override;
        void Update(float dt);
        void Draw(int screenW, int screenH);

    private:
        unsigned texSplash = 0;
        unsigned texFaceHappy = 0;
        unsigned texFaceUpset = 0;
        unsigned texBottleFull = 0;
        unsigned texBottleBreak = 0;
        unsigned texBottleBroken{ 0 };

        int displayedHealth = 100;
        PlayerHealthComponent* health = nullptr;

        struct BottleState
        {
            bool isBroken = false;
            float breakAnimTimer = 0.0f;
            bool isVisible = true;
        };

        std::array<BottleState, 5> bottles{};

        static constexpr float BREAK_ANIM_DURATION = 0.4f;
        static constexpr int BREAK_FRAMES = 3;

        void LoadTextures();
        void ResetBottles();
        void SyncFromHealth();
    };
}
