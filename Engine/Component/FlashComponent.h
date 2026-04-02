/*********************************************************************************************
 \file      FlashComponent.h
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Declares the FlashComponent used to drive temporary visibility flashing on
            renderable objects.

 \details   FlashComponent stores the authored flash settings together with lightweight
            runtime state used by game-side logic to toggle an object's RenderComponent.
            In BloodyGoodCurry it is primarily used for objects that should flicker into
            view after an enemy-clear condition is met, while preserving the object's
            original render colors, alpha, visibility, and blend mode.

            Responsibilities:
            - Store authored flash timing and activation settings loaded from data.
            - Track transient runtime state such as elapsed flash time and completion.
            - Cache the original RenderComponent state so the effect can restore it safely.
            - Support cloning and serialization for prefab and level workflows.

 \copyright
            All content 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once

#include "Composition/Component.h"
#include "Component/RenderComponent.h"
#include "Memory/ComponentPool.h"
#include "Serialization/Serialization.h"

namespace Framework
{
    /*****************************************************************************************
      \class FlashComponent
      \brief Component that stores configuration and runtime state for a visibility flash effect.

      FlashComponent does not update itself directly. Instead, gameplay systems read the
      values stored here to decide when the owner should flash, whether it begins visible
      or hidden, and when the effect has completed. The component also caches the owner's
      original RenderComponent values so the visual state can be restored after flashing.
    *****************************************************************************************/
    class FlashComponent : public GameComponent
    {
    public:
        float frequency{ 8.0f };            ///< Flash toggle rate in Hz.
        float duration{ 0.0f };             ///< Total flash duration in seconds; 0 means indefinite.
        bool start_visible{ true };         ///< True when the first flash half-cycle should be visible.
        bool activate_on_enemy_clear{ true }; ///< Enables the enemy-clear activation path used by gameplay.
        bool hide_until_activated{ false }; ///< Keeps the owner hidden until flashing begins.

        // Runtime state.
        float timer{ 0.0f };                ///< Elapsed flash time accumulated by runtime update code.
        bool visible{ true };               ///< Current logical visibility state of the flash effect.
        bool flashing{ false };             ///< True while the effect is actively toggling visibility.
        bool completed{ false };            ///< True once a finite-duration flash has finished.
        bool hasCachedRenderState{ false }; ///< True once the original RenderComponent state has been saved.
        float cachedR{ 1.0f };              ///< Cached render red channel for restoration.
        float cachedG{ 1.0f };              ///< Cached render green channel for restoration.
        float cachedB{ 1.0f };              ///< Cached render blue channel for restoration.
        float cachedA{ 1.0f };              ///< Cached render alpha channel for restoration.
        bool cachedVisible{ true };         ///< Cached render visibility flag for restoration.
        BlendMode cachedBlendMode{ BlendMode::Alpha }; ///< Cached blend mode for restoration.

        /*************************************************************************************
          \brief Initializes the component's transient runtime state.
          \details
              - Resets the flash timer.
              - Restores the authored starting visibility.
              - Clears runtime completion and cached-render flags.
        *************************************************************************************/
        void initialize() override
        {
            timer = 0.0f;
            visible = start_visible;
            flashing = false;
            completed = false;
            hasCachedRenderState = false;
        }

        /*************************************************************************************
          \brief Handles incoming messages sent to this component.
          \param m Reference to the incoming message.
          \note Currently unused; kept for ECS interface compatibility.
        *************************************************************************************/
        void SendMessage(Message& m) override { (void)m; }

        /*************************************************************************************
          \brief Serializes authored flash settings from prefab or level data.
          \param s Reference to the serializer.
          \details Reads the following keys when present:
            - "frequency"
            - "duration"
            - "start_visible"
            - "activate_on_enemy_clear"
            - "hide_until_activated"
          Runtime-only fields are intentionally not serialized.
        *************************************************************************************/
        void Serialize(ISerializer& s) override
        {
            if (s.HasKey("frequency"))
                StreamRead(s, "frequency", frequency);
            if (s.HasKey("duration"))
                StreamRead(s, "duration", duration);
            if (s.HasKey("start_visible"))
                StreamRead(s, "start_visible", start_visible);
            if (s.HasKey("activate_on_enemy_clear"))
                StreamRead(s, "activate_on_enemy_clear", activate_on_enemy_clear);
            if (s.HasKey("hide_until_activated"))
                StreamRead(s, "hide_until_activated", hide_until_activated);
        }

        /*************************************************************************************
          \brief Creates a deep copy of this component for prefab instancing.
          \return A component handle holding the cloned FlashComponent.
          \details Authored settings are copied, while transient runtime state is reset so
                   newly spawned instances begin from a clean flashing state.
        *************************************************************************************/
        ComponentHandle Clone() const override
        {
            auto copy = ComponentPool<FlashComponent>::CreateTyped();
            copy->frequency = frequency;
            copy->duration = duration;
            copy->start_visible = start_visible;
            copy->activate_on_enemy_clear = activate_on_enemy_clear;
            copy->hide_until_activated = hide_until_activated;
            copy->timer = 0.0f;
            copy->visible = start_visible;
            copy->flashing = false;
            copy->completed = false;
            copy->hasCachedRenderState = false;
            copy->cachedR = 1.0f;
            copy->cachedG = 1.0f;
            copy->cachedB = 1.0f;
            copy->cachedA = 1.0f;
            copy->cachedVisible = true;
            copy->cachedBlendMode = BlendMode::Alpha;
            return copy;
        }
    };
}
