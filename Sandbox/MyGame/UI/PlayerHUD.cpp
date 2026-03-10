/*********************************************************************************************
 \file      PlayerHUD.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Author, adapted for game-side build ownership
 \brief     Game-side implementation of Framework::PlayerHUDComponent.
 \details   The component declaration remains in Engine/ for serialization and ECS
            registration, but BloodyGoodCurry now owns the HUD behavior/render code so
            project-specific UI logic is not compiled into the engine target.
*********************************************************************************************/

#include "Components/PlayerHUD.h"

#include "Components/PlayerHealthComponent.h"
#include "Core/PathUtils.h"
#include "EngineCall.hpp"
#include "Resource_Asset_Manager/Resource_Manager.h"

#include <algorithm>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "Common/CRTDebug.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif

namespace Framework
{
    namespace
    {
        /*****************************************************************************************
         \brief Loads a HUD texture from the active game project's asset folder.
         \param name Cache key used by Resource_Manager.
         \param path Relative asset path inside the project's Assets directory.
         \return Loaded texture handle, or 0 if the texture could not be loaded.
         \details
            - Resolves the path through the current project asset root.
            - Loads the texture through Resource_Manager so repeated calls reuse the cache.
            - Prints a debug message when loading fails.
        *****************************************************************************************/
        unsigned LoadTexture(const char* name, const char* path)
        {
            std::string resolved = ResolveAssetPath(path).string();
            if (!Resource_Manager::load(name, resolved))
            {
                std::cout << "[HUD] Failed to load " << name << "\n";
                return 0u;
            }
            return Resource_Manager::getTexture(name);
        }
    }

    /*****************************************************************************************
     \brief Initializes the HUD component for its owning player object.
     \details
        - Caches the owner's PlayerHealthComponent.
        - Loads all HUD textures from the active game project.
        - Resets bottle animation state and syncs the initial display to current health.
     \note
        If the owner has no PlayerHealthComponent, the HUD stays inactive until one exists.
    *****************************************************************************************/
    void PlayerHUDComponent::initialize()
    {
        health = GetOwner()
            ? GetOwner()->GetComponentType<PlayerHealthComponent>(ComponentTypeId::CT_PlayerHealthComponent)
            : nullptr;

        LoadTextures();
        ResetBottles();
        SyncFromHealth();
    }

    /*****************************************************************************************
     \brief Receives ECS messages for the HUD component.
     \param m Incoming message.
     \note
        The current HUD implementation does not react to messages, so the argument is ignored.
    *****************************************************************************************/
    void PlayerHUDComponent::SendMessage(Message& m)
    {
        (void)m;
    }

    /*****************************************************************************************
     \brief Serializes or deserializes HUD state.
     \param s Active serializer.
     \note
        The HUD is currently configured entirely in code, so no fields are read or written here.
    *****************************************************************************************/
    void PlayerHUDComponent::Serialize(ISerializer& s)
    {
        (void)s;
    }

    /*****************************************************************************************
     \brief Clones the HUD component for prefab duplication or copied objects.
     \return A new PlayerHUDComponent with the current display state copied over.
     \details
        - Preserves the displayed health snapshot.
        - Preserves bottle animation and broken-state flags.
        - Runtime pointers such as the owner health cache are re-established during initialize().
    *****************************************************************************************/
    ComponentHandle PlayerHUDComponent::Clone() const
    {
        auto copy = ComponentPool<PlayerHUDComponent>::CreateTyped();
        copy->displayedHealth = displayedHealth;
        copy->bottles = bottles;
        return copy;
    }

    /*****************************************************************************************
     \brief Loads all textures required by the health HUD.
     \details
        - Loads the splash background, face icons, full bottle, broken bottle, and break VFX.
        - Uses project-relative asset paths so each game can own its own HUD art.
    *****************************************************************************************/
    void PlayerHUDComponent::LoadTextures()
    {
        texSplash = LoadTexture("hud_splash", "Textures/UI/Health Bar/Health_splash.png");
        texFaceHappy = LoadTexture("hud_face_happy", "Textures/UI/Health Bar/Health_HappyFace.png");
        texFaceUpset = LoadTexture("hud_face_upset", "Textures/UI/Health Bar/Health_UpsetFace.png");
        texBottleFull = LoadTexture("hud_bottle", "Textures/UI/Health Bar/Health_Life.png");
        texBottleBreak = LoadTexture("hud_bottle_break", "Textures/UI/Health Bar/Broken_Life_VFX_Sprite.png");
        texBottleBroken = LoadTexture("hud_bottle_broken", "Textures/UI/Health Bar/Health_BrokenLife.png");
        texMeleeReady = LoadTexture("hud_melee_ready", "Textures/UI/Melee.png");
        texMeleeCooldown = LoadTexture("hud_melee_cooldown", "Textures/UI/No melee.png");
        texRangeReady = LoadTexture("hud_range_ready", "Textures/UI/Range.png");
        texRangeCooldown = LoadTexture("hud_range_cooldown", "Textures/UI/No range.png");
        texTalismanReady = LoadTexture("hud_talisman_ready", "Textures/UI/Tailsman.png");
        texTalismanCooldown = LoadTexture("hud_talisman_cooldown", "Textures/UI/No tailsman.png");
        texBubble1 = LoadTexture("hud_bubble_1", "Textures/UI/Bubble count/Bubble1.png");
        texBubble1Appear = LoadTexture("hud_bubble_1_appear", "Textures/UI/Bubble count/Bubble1_appear.png");
        texBubble1Pop = LoadTexture("hud_bubble_1_pop", "Textures/UI/Bubble count/Bubble1_pop.png");
        texBubble2 = LoadTexture("hud_bubble_2", "Textures/UI/Bubble count/Bubble2.png");
        texBubble2Appear = LoadTexture("hud_bubble_2_appear", "Textures/UI/Bubble count/Bubble2_appear.png");
        texBubble2Pop = LoadTexture("hud_bubble_2_pop", "Textures/UI/Bubble count/Bubble2_pop.png");
    }

    /*****************************************************************************************
     \brief Resets all bottle display state to the default healthy state.
     \details
        - Marks every bottle visible.
        - Clears broken-state flags.
        - Clears any active break-animation timers.
    *****************************************************************************************/
    void PlayerHUDComponent::ResetBottles()
    {
        for (auto& b : bottles)
        {
            b.isBroken = false;
            b.isVisible = true;
            b.breakAnimTimer = 0.0f;
        }
    }

    /*****************************************************************************************
     \brief Rebuilds bottle visibility from the player's current health value.
     \details
        - Clamps health to a valid range using the player's max health.
        - Converts numeric health into a 0-5 bottle count.
        - Reinitializes bottle state so the display matches health immediately.
     \note
        This is used for initial sync, not animated transitions.
    *****************************************************************************************/
    void PlayerHUDComponent::SyncFromHealth()
    {
        if (!health)
            return;

        const int maxHealth = std::max(1, health->playerMaxhealth);
        const int currentHealth = std::clamp(health->playerHealth, 0, maxHealth);

        const int bottleCount = (currentHealth * 5) / maxHealth;

        displayedHealth = currentHealth;
        ResetBottles();

        for (int i = 0; i < 5; ++i)
        {
            bottles[i].isBroken = i >= bottleCount;
            bottles[i].isVisible = !bottles[i].isBroken;
        }
    }

    /*****************************************************************************************
     \brief Updates the HUD's animated response to health changes.
     \param dt Delta time in seconds.
     \details
        - Detects health loss and starts bottle-break animations.
        - Detects health gain and restores bottles immediately.
        - Advances break-animation timers and hides bottles when their break animation completes.
     \note
        If no PlayerHealthComponent is cached, this function does nothing.
    *****************************************************************************************/
    void PlayerHUDComponent::Update(float dt)
    {
        UpdateAbilityBubbleStates(dt);

        if (!health)
            return;

        const int maxHealth = std::max(1, health->playerMaxhealth);
        const int currentHealth = std::clamp(health->playerHealth, 0, maxHealth);

        const int oldBottleCount = (displayedHealth * 5) / maxHealth;
        const int newBottleCount = (currentHealth * 5) / maxHealth;

        if (newBottleCount < oldBottleCount)
        {
            for (int i = oldBottleCount - 1; i >= newBottleCount; --i)
            {
                auto& b = bottles[i];
                if (!b.isBroken)
                {
                    b.isBroken = true;
                    b.breakAnimTimer = BREAK_ANIM_DURATION;
                }
            }
        }

        if (newBottleCount > oldBottleCount)
        {
            for (int i = 0; i < newBottleCount; ++i)
            {
                auto& b = bottles[i];
                b.isBroken = false;
                b.isVisible = true;
                b.breakAnimTimer = 0.0f;
            }
        }

        displayedHealth = currentHealth;

        for (auto& b : bottles)
        {
            if (b.breakAnimTimer > 0.0f)
            {
                b.breakAnimTimer -= dt;
                if (b.breakAnimTimer <= 0.0f)
                {
                    b.breakAnimTimer = 0.0f;
                    b.isVisible = false;
                }
            }
        }

        displayedHealth = currentHealth;

        for (auto& b : bottles)
        {
            if (b.breakAnimTimer > 0.0f)
            {
                b.breakAnimTimer -= dt;
                if (b.breakAnimTimer <= 0.0f)
                {
                    b.breakAnimTimer = 0.0f;
                }
            }
        }
    }

    void PlayerHUDComponent::UpdateAbilityBubbleStates(float dt)
    {
        const mygame::PlayerAbilityHudState abilityHudState =
            mygame::GetPlayerAbilityHudState(GetOwner());

        const std::array<const mygame::AbilityCooldownUiState*, 3> cooldownStates{ {
            &abilityHudState.talisman,
            &abilityHudState.ranged,
            &abilityHudState.melee
        } };
        const std::array<int, 3> maxBubbleCounts{ {
            COOLDOWN_BUBBLE_STEPS,
            COOLDOWN_BUBBLE_STEPS,
            1
        } };

        for (std::size_t i = 0; i < cooldownStates.size(); ++i)
        {
            const auto& state = *cooldownStates[i];
            int targetCount = 0;

            if (!state.ready && state.duration > 0.0f && state.remaining > 0.0f)
            {
                const float remainingRatio = std::clamp(state.remaining / state.duration, 0.0f, 1.0f);
                targetCount = (remainingRatio > 0.5f) ? maxBubbleCounts[i] : 1;
            }

            AdvanceAbilityBubbleState(abilityBubbleStates[i], targetCount, dt);
            AdvanceAbilityIconState(abilityIconStates[i], state.ready, state.remaining, state.duration);
        }
    }

    void PlayerHUDComponent::AdvanceAbilityBubbleState(AbilityBubbleState& bubbleState, int targetCount, float dt)
    {
        bubbleState.pendingCount = std::clamp(targetCount, 0, COOLDOWN_BUBBLE_STEPS);

        if (bubbleState.animKind != BubbleAnimKind::None)
        {
            bubbleState.animTimer = std::max(0.0f, bubbleState.animTimer - dt);
            if (bubbleState.animTimer > 0.0f)
                return;

            if (bubbleState.animKind == BubbleAnimKind::Pop)
            {
                bubbleState.displayedCount = 0;
                bubbleState.animKind = BubbleAnimKind::None;

                if (bubbleState.pendingCount > 0)
                {
                    bubbleState.displayedCount = bubbleState.pendingCount;
                    bubbleState.animKind = BubbleAnimKind::Appear;
                    bubbleState.animTimer = BUBBLE_ANIM_DURATION;
                    return;
                }
            }
            else
            {
                bubbleState.animKind = BubbleAnimKind::None;
            }
        }

        if (bubbleState.displayedCount == bubbleState.pendingCount)
            return;

        if (bubbleState.displayedCount > 0)
        {
            bubbleState.animKind = BubbleAnimKind::Pop;
            bubbleState.animTimer = BUBBLE_ANIM_DURATION;
            return;
        }

        if (bubbleState.pendingCount > 0)
        {
            bubbleState.displayedCount = bubbleState.pendingCount;
            bubbleState.animKind = BubbleAnimKind::Appear;
            bubbleState.animTimer = BUBBLE_ANIM_DURATION;
        }
    }

    void PlayerHUDComponent::AdvanceAbilityIconState(AbilityIconState& iconState, bool ready, float remaining, float duration)
    {
        if (ready || duration <= 0.0f || remaining <= 0.0f)
        {
            iconState.cooldownBlend = 1.0f;
            return;
        }

        iconState.cooldownBlend = std::clamp(1.0f - (remaining / duration), 0.0f, 1.0f);
    }

    /*****************************************************************************************
     \brief Draws the player's health HUD in screen-space.
     \param screenW Current viewport width in pixels.
     \param screenH Current viewport height in pixels.
     \details
        - Scales HUD positions and sizes against a 720p reference height.
        - Draws the splash background, mood face, and bottle icons.
        - Uses the break sprite sheet while bottles are animating.
        - Restores the graphics view/projection state after UI drawing.
    *****************************************************************************************/
    void PlayerHUDComponent::Draw(int screenW, int screenH)
    {
        using namespace gfx;

        const float refHeight = 720.0f;
        const float scaleFactor = static_cast<float>(screenH) / refHeight;

        const float startX = 20.0f * scaleFactor;
        const float startY = static_cast<float>(screenH) - (150.0f * scaleFactor);
        const float splashW = 250.0f * scaleFactor;
        const float splashH = 120.0f * scaleFactor;
        const float splashX = startX + (90.0f * scaleFactor);
        const float splashY = startY - (10.0f * scaleFactor);

        if (texSplash)
        {
            Graphics::renderSpriteUI(texSplash, splashX, splashY, splashW, splashH, 1, 1, 1, 1, screenW, screenH);
        }

        float healthPercent = 0.0f;
        if (health && health->playerMaxhealth > 0)
            healthPercent = (static_cast<float>(displayedHealth) / health->playerMaxhealth) * 100.0f;
        const unsigned faceTex = (healthPercent >= 50.0f) ? texFaceHappy : texFaceUpset;

        const float faceW = 110.0f * scaleFactor;
        const float faceH = 100.0f * scaleFactor;
        const float faceX = startX + (10.0f * scaleFactor);
        const float faceY = startY + (20.0f * scaleFactor);

        if (faceTex)
            Graphics::renderSpriteUI(faceTex, faceX, faceY, faceW, faceH, 1, 1, 1, 1, screenW, screenH);

        const float bottleW = 45.0f * scaleFactor;
        const float bottleH = 70.0f * scaleFactor;
        const float bottleSpacing = -10.0f * scaleFactor;
        const float bottleStartX = faceX + faceW - (10.0f * scaleFactor);
        const float bottleY = faceY + (5.0f * scaleFactor);
        const mygame::PlayerAbilityHudState abilityHudState =
            mygame::GetPlayerAbilityHudState(GetOwner());

        glm::mat4 uiOrtho = glm::ortho(0.0f, static_cast<float>(screenW),
            0.0f, static_cast<float>(screenH),
            -1.0f, 1.0f);

        Graphics::setViewProjection(glm::mat4(1.0f), uiOrtho);

        for (int i = 0; i < static_cast<int>(bottles.size()); ++i)
        {
            const float xPos = bottleStartX + (i * (bottleW + bottleSpacing));
            const auto& b = bottles[static_cast<std::size_t>(i)];

            const bool debugThisBottle = (i == 2);
            float yPos = bottleY;

            if (i >= 2)
            {
                yPos -= (5.0f * scaleFactor);
            }

            if (b.breakAnimTimer > 0.0f)
            {
                if (debugThisBottle)
                    std::cout << "[HUD] Bottle 2 is ANIMATING. Timer: " << b.breakAnimTimer << "\n";

                float timePercent = 1.0f - (b.breakAnimTimer / BREAK_ANIM_DURATION);
                int frame = static_cast<int>(timePercent * BREAK_FRAMES);
                frame = std::clamp(frame, 0, BREAK_FRAMES - 1);

                if (texBottleBreak)
                {
                    Graphics::renderSpriteFrame(
                        texBottleBreak, xPos + bottleW / 2, yPos + bottleH / 2, 0.0f, bottleW, bottleH, frame, 3, 1);
                }
            }
            else if (b.isBroken)
            {
                if (debugThisBottle)
                    std::cout << "[HUD] Bottle 2 is BROKEN. Texture ID: " << texBottleBroken << "\n";

                if (texBottleBroken != 0)
                {
                    Graphics::renderSpriteFrame(
                        texBottleBroken, xPos + bottleW / 2, yPos + bottleH / 2, 0.0f, bottleW, bottleH, 0, 1, 1);
                }
                else if (texBottleFull)
                {
                    Graphics::renderSpriteFrame(
                        texBottleFull, xPos + bottleW / 2, yPos + bottleH / 2, 0.0f, bottleW, bottleH,
                        0, 1, 1, 1.0f, 0.0f, 1.0f, 1.0f);
                }
            }
            else
            {
                if (texBottleFull)
                {
                    Graphics::renderSpriteFrame(
                        texBottleFull, xPos + bottleW / 2, yPos + bottleH / 2, 0.0f, bottleW, bottleH, 0, 1, 1);
                }
            }
        }

        const float panelWidth = 110.0f * scaleFactor;
        const float panelHeight = 300.0f * scaleFactor;
        const float panelX = static_cast<float>(screenW) - panelWidth - (24.0f * scaleFactor);
        const float panelY = 30.0f * scaleFactor;

        struct AbilityIconDraw
        {
            const mygame::AbilityCooldownUiState& state;
            unsigned readyTexture;
            unsigned cooldownTexture;
        };

        const std::array<AbilityIconDraw, 3> abilityIcons{ {
            { abilityHudState.talisman, texTalismanReady, texTalismanCooldown },
            { abilityHudState.ranged, texRangeReady, texRangeCooldown },
            { abilityHudState.melee, texMeleeReady, texMeleeCooldown }
        } };

        const float iconSize = 74.0f * scaleFactor;
        const float iconGap = 18.0f * scaleFactor;
        const float iconX = panelX + ((panelWidth - iconSize) * 0.5f);
        const float iconTopY = panelY + panelHeight - iconSize - (18.0f * scaleFactor);
        constexpr float kOrangeIconScale = 1.10f;
        constexpr float kNonOrangeIconScale = 0.92f;

        for (std::size_t i = 0; i < abilityIcons.size(); ++i)
        {
            const auto& icon = abilityIcons[i];
            const auto& bubbleState = abilityBubbleStates[i];
            const auto& iconState = abilityIconStates[i];
            const float iconY = iconTopY - (static_cast<float>(i) * (iconSize + iconGap));
            const float cooldownBlend = std::clamp(iconState.cooldownBlend, 0.0f, 1.0f);
            const float iconCenterX = iconX + (iconSize * 0.5f);
            const float iconCenterY = iconY + (iconSize * 0.5f);

            auto drawCenteredIcon = [&](unsigned texture, float alpha, float scale)
                {
                    if (texture == 0u || alpha <= 0.0f)
                        return;

                    const float drawSize = iconSize * scale;
                    Graphics::renderSpriteUI(
                        texture,
                        iconCenterX - (drawSize * 0.5f),
                        iconCenterY - (drawSize * 0.5f),
                        drawSize,
                        drawSize,
                        1.0f, 1.0f, 1.0f, alpha,
                        screenW, screenH);
                };

            auto drawPartialIcon = [&](unsigned texture, float scale, float yOffsetUv, float yScaleUv)
                {
                    if (texture == 0u || yScaleUv <= 0.0f)
                        return;

                    const float drawSize = iconSize * scale;
                    const float drawX = iconCenterX - (drawSize * 0.5f);
                    const float drawY = iconCenterY - (drawSize * 0.5f);
                    const float drawHeight = drawSize * yScaleUv;

                    if (drawHeight <= 0.0f)
                        return;

                    Graphics::SpriteInstance fillInstance{};
                    fillInstance.model = glm::mat4(1.0f);
                    fillInstance.model = glm::translate(
                        fillInstance.model,
                        glm::vec3(
                            drawX + (drawSize * 0.5f),
                            drawY + (drawSize * yOffsetUv) + (drawHeight * 0.5f),
                            0.0f));
                    fillInstance.model = glm::scale(
                        fillInstance.model,
                        glm::vec3(drawSize, drawHeight, 1.0f));
                    fillInstance.tint = glm::vec4(1.0f);
                    fillInstance.uv = glm::vec4(0.0f, yOffsetUv, 1.0f, yScaleUv);

                    Graphics::renderSpriteBatchInstanced(
                        texture,
                        &fillInstance,
                        1);
                };

            if (cooldownBlend <= 0.0f)
            {
                drawCenteredIcon(icon.cooldownTexture, 1.0f, kNonOrangeIconScale);
            }
            else if (cooldownBlend >= 1.0f)
            {
                drawCenteredIcon(icon.readyTexture, 1.0f, kOrangeIconScale);
            }
            else
            {
                drawPartialIcon(icon.cooldownTexture, kNonOrangeIconScale, cooldownBlend, 1.0f - cooldownBlend);
                drawPartialIcon(icon.readyTexture, kOrangeIconScale, 0.0f, cooldownBlend);
            }

            if (bubbleState.displayedCount > 0)
            {
                const float bubbleSize = 80.0f * scaleFactor;
                const float bubbleX = iconX + iconSize - (bubbleSize * 0.75f);
                const float bubbleY = iconY + iconSize - (bubbleSize * 0.70f);
                int bubbleCountToDraw = bubbleState.displayedCount;
                BubbleAnimKind bubbleAnimKind = bubbleState.animKind;

                auto selectBubbleTexture = [&](int count, BubbleAnimKind animKind) -> unsigned
                    {
                        if (count == 2)
                        {
                            if (animKind == BubbleAnimKind::Appear)
                                return texBubble2Appear;
                            if (animKind == BubbleAnimKind::Pop)
                                return texBubble2Pop;
                            return texBubble2;
                        }

                        if (animKind == BubbleAnimKind::Appear)
                            return texBubble1Appear;
                        if (animKind == BubbleAnimKind::Pop)
                            return texBubble1Pop;
                        return texBubble1;
                    };

                const unsigned bubbleTexture =
                    selectBubbleTexture(bubbleCountToDraw, bubbleAnimKind);

                if (bubbleTexture == 0u)
                    continue;

                if (bubbleAnimKind == BubbleAnimKind::None)
                {
                    Graphics::renderSpriteUI(
                        bubbleTexture, bubbleX, bubbleY, bubbleSize, bubbleSize,
                        1.0f, 1.0f, 1.0f, 1.0f, screenW, screenH);
                    continue;
                }

                const float animProgress = 1.0f - (bubbleState.animTimer / BUBBLE_ANIM_DURATION);
                const int frame = std::clamp(
                    static_cast<int>(animProgress * static_cast<float>(BUBBLE_ANIM_FRAMES)),
                    0,
                    BUBBLE_ANIM_FRAMES - 1);

                Graphics::renderSpriteFrame(
                    bubbleTexture,
                    bubbleX + (bubbleSize * 0.5f),
                    bubbleY + (bubbleSize * 0.5f),
                    0.0f,
                    bubbleSize,
                    bubbleSize,
                    frame,
                    BUBBLE_ANIM_FRAMES,
                    1);
            }
        }

        Graphics::resetViewProjection();
    }
}
