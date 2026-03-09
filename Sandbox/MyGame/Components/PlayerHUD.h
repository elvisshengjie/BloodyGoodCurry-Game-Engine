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
        unsigned texMeleeReady{ 0 };
        unsigned texMeleeCooldown{ 0 };
        unsigned texRangeReady{ 0 };
        unsigned texRangeCooldown{ 0 };
        unsigned texTalismanReady{ 0 };
        unsigned texTalismanCooldown{ 0 };
        unsigned texBubble1{ 0 };
        unsigned texBubble1Appear{ 0 };
        unsigned texBubble1Pop{ 0 };
        unsigned texBubble2{ 0 };
        unsigned texBubble2Appear{ 0 };
        unsigned texBubble2Pop{ 0 };

        int displayedHealth = 100;
        PlayerHealthComponent* health = nullptr;

        enum class BubbleAnimKind
        {
            None,
            Appear,
            Pop
        };

        struct AbilityBubbleState
        {
            int displayedCount{ 0 };
            int pendingCount{ 0 };
            BubbleAnimKind animKind{ BubbleAnimKind::None };
            float animTimer{ 0.0f };
        };

        struct AbilityIconState
        {
            float cooldownBlend{ 0.0f };
        };

        struct BottleState
        {
            bool isBroken = false;
            float breakAnimTimer = 0.0f;
            bool isVisible = true;
        };

        std::array<BottleState, 5> bottles{};
        std::array<AbilityBubbleState, 3> abilityBubbleStates{};
        std::array<AbilityIconState, 3> abilityIconStates{};

        static constexpr float BREAK_ANIM_DURATION = 0.4f;
        static constexpr int BREAK_FRAMES = 3;
        static constexpr int COOLDOWN_BUBBLE_STEPS = 2;
        static constexpr int BUBBLE_ANIM_FRAMES = 7;
        static constexpr float BUBBLE_ANIM_DURATION = 0.28f;

        void LoadTextures();
        void ResetBottles();
        void SyncFromHealth();
        void UpdateAbilityBubbleStates(float dt);
        void AdvanceAbilityBubbleState(AbilityBubbleState& bubbleState, int targetCount, float dt);
        void AdvanceAbilityIconState(AbilityIconState& iconState, bool ready, float remaining, float duration);
    };
}
