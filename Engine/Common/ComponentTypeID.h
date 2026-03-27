/*********************************************************************************************
 \file      ComponentType.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Defines the ComponentTypeId enumeration which uniquely identifies each
            component type within the ECS framework. Used for registration, lookup, and
            retrieval of components from GameObjectComposition instances.

 \copyright
            All content � 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

#include <compare>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Framework
{
    /*****************************************************************************************
      \struct ComponentTypeId
      \brief Lightweight runtime component identifier wrapper.

      The old enum-based list has been replaced by an integral wrapper so the engine can
      keep stable built-in IDs while also allowing games to register additional component
      types at runtime through ComponentTypeRegistry.
    *****************************************************************************************/
    struct ComponentTypeId
    {
        using Storage = std::uint16_t;

        constexpr ComponentTypeId() noexcept = default;
        constexpr ComponentTypeId(Storage rawValue) noexcept : value(rawValue) {}
        constexpr operator Storage() const noexcept { return value; }

        constexpr auto operator<=>(const ComponentTypeId&) const noexcept = default;

        Storage value{ 0 };

        static constexpr Storage CT_None = 0;

        // Engine-owned IDs. These stay stable across games.
        static constexpr Storage CT_TransformComponent = 1;
        static constexpr Storage CT_RenderComponent = 2;
        static constexpr Storage CT_CircleRenderComponent = 3;
        static constexpr Storage CT_InputComponents = 4;
        static constexpr Storage CT_RigidBodyComponent = 5;
        static constexpr Storage CT_HitBoxComponent = 6;
        static constexpr Storage CT_SpriteComponent = 7;
        static constexpr Storage CT_SpriteAnimationComponent = 8;
        static constexpr Storage CT_ShadowComponent = 9;
        static constexpr Storage CT_BehaviorTreeComponent = 10;
        static constexpr Storage CT_AudioComponent = 11;
        static constexpr Storage CT_BehaviourComponent = 12;
        static constexpr Storage CT_FlashComponent = 13;

        // Legacy fixed values retained so existing BloodyGoodCurry data can reserve the
        // same slots from the game side while the engine/game split is being migrated.
        static constexpr Storage CT_GlowComponent = 32;
        static constexpr Storage CT_PlayerComponent = 33;
        static constexpr Storage CT_PlayerHealthComponent = 34;
        static constexpr Storage CT_PlayerAttackComponent = 35;
        static constexpr Storage CT_PlayerHUDComponent = 36;
        static constexpr Storage CT_EnemyComponent = 37;
        static constexpr Storage CT_EnemyDecisionTreeComponent = 38;
        static constexpr Storage CT_EnemyAttackComponent = 39;
        static constexpr Storage CT_EnemyHealthComponent = 40;
        static constexpr Storage CT_EnemyTypeComponent = 41;
        static constexpr Storage CT_WayPointComponent = 42;
        static constexpr Storage CT_ZoomTriggerComponent = 43;
        static constexpr Storage CT_GateTargetComponent = 44;

        static constexpr Storage CT_FirstRuntimeComponent = 128;
    };

    /*****************************************************************************************
      \class ComponentTypeRegistry
      \brief Runtime registry mapping component names to component IDs.

      The registry keeps a dense vector of names indexed by component ID so a game can
      reserve or free project-specific component slots without modifying engine enums.
    *****************************************************************************************/
    class ComponentTypeRegistry
    {
    public:
        /*************************************************************************************
          \brief Returns the process-wide component type registry singleton.
        *************************************************************************************/
        static ComponentTypeRegistry& Get()
        {
            static ComponentTypeRegistry registry;
            return registry;
        }

        /*************************************************************************************
          \brief Registers a component name and returns its runtime ID.
          \param name         Stable component name used by serialization and factory lookup.
          \param preferredId  Optional fixed ID to reserve for the component.
          \return The resolved component ID for the registered name.
          \details
            - Reuses the existing ID if the name is already registered.
            - Reserves the requested ID when \p preferredId is supplied.
            - Allocates the next free runtime ID otherwise.
        *************************************************************************************/
        ComponentTypeId Register(std::string_view name,
            std::optional<ComponentTypeId> preferredId = std::nullopt)
        {
            if (name.empty())
                throw std::invalid_argument("ComponentTypeRegistry::Register requires a non-empty name");

            const std::string key(name);
            if (const auto existing = nameToId.find(key); existing != nameToId.end())
            {
                return ComponentTypeId(existing->second);
            }

            ComponentTypeId::Storage resolved = 0;
            if (preferredId.has_value())
            {
                resolved = preferredId->value;
                ensureSize(resolved);
                if (!idToName[resolved].empty())
                {
                    throw std::runtime_error("ComponentTypeRegistry::Register id collision for " + key);
                }
            }
            else
            {
                resolved = FindNextFreeId();
                ensureSize(resolved);
            }

            idToName[resolved] = key;
            nameToId.emplace(key, resolved);
            return ComponentTypeId(resolved);
        }

        /*************************************************************************************
          \brief Removes a runtime-registered component ID from the registry.
          \param id Component ID to release.
          \return True if a runtime ID was removed, false otherwise.
          \note Built-in engine IDs are intentionally not removable.
        *************************************************************************************/
        bool Unregister(ComponentTypeId id)
        {
            if (id.value >= idToName.size())
                return false;

            if (id.value < ComponentTypeId::CT_FirstRuntimeComponent)
                return false;

            std::string& entry = idToName[id.value];
            if (entry.empty())
                return false;

            nameToId.erase(entry);
            entry.clear();
            return true;
        }

        /*************************************************************************************
          \brief Looks up a registered component ID by name.
          \param name Component name to resolve.
          \return The matching component ID, or std::nullopt if the name is unknown.
        *************************************************************************************/
        std::optional<ComponentTypeId> Find(std::string_view name) const
        {
            if (const auto it = nameToId.find(std::string(name)); it != nameToId.end())
                return ComponentTypeId(it->second);
            return std::nullopt;
        }

        /*************************************************************************************
          \brief Returns the registered name associated with a component ID.
          \param id Component ID to inspect.
          \return Component name, or an empty view when the ID is unknown.
        *************************************************************************************/
        std::string_view Name(ComponentTypeId id) const
        {
            if (id.value >= idToName.size())
                return {};
            return idToName[id.value];
        }

        /*************************************************************************************
          \brief Reports whether the given ID currently maps to a registered component.
          \param id Component ID to test.
          \return True when the ID is present in the registry.
        *************************************************************************************/
        bool IsRegistered(ComponentTypeId id) const
        {
            return id.value < idToName.size() && !idToName[id.value].empty();
        }

    private:
        /*************************************************************************************
          \brief Seeds the registry with engine-owned fixed component IDs.
          \details Game-specific IDs are registered by each game project at runtime.
        *************************************************************************************/
        ComponentTypeRegistry()
        {
            registerFixed(ComponentTypeId::CT_TransformComponent, "TransformComponent");
            registerFixed(ComponentTypeId::CT_RenderComponent, "RenderComponent");
            registerFixed(ComponentTypeId::CT_CircleRenderComponent, "CircleRenderComponent");
            registerFixed(ComponentTypeId::CT_InputComponents, "InputComponents");
            registerFixed(ComponentTypeId::CT_RigidBodyComponent, "RigidBodyComponent");
            registerFixed(ComponentTypeId::CT_HitBoxComponent, "HitBoxComponent");
            registerFixed(ComponentTypeId::CT_SpriteComponent, "SpriteComponent");
            registerFixed(ComponentTypeId::CT_SpriteAnimationComponent, "SpriteAnimationComponent");
            registerFixed(ComponentTypeId::CT_ShadowComponent, "ShadowComponent");
            registerFixed(ComponentTypeId::CT_BehaviorTreeComponent, "BehaviorTreeComponent");
            registerFixed(ComponentTypeId::CT_AudioComponent, "AudioComponent");
            registerFixed(ComponentTypeId::CT_BehaviourComponent, "BehaviourComponent");
            registerFixed(ComponentTypeId::CT_FlashComponent, "FlashComponent");

        }

        /*************************************************************************************
          \brief Expands the ID-to-name table so the requested ID can be indexed safely.
          \param id Component ID that must fit inside the dense lookup vector.
        *************************************************************************************/
        void ensureSize(ComponentTypeId::Storage id)
        {
            if (id >= idToName.size())
                idToName.resize(static_cast<std::size_t>(id) + 1);
        }

        /*************************************************************************************
          \brief Inserts a fixed component ID/name mapping during registry bootstrap.
          \param id    Reserved component ID to bind.
          \param name  Stable serialized name for the component.
        *************************************************************************************/
        void registerFixed(ComponentTypeId::Storage id, std::string_view name)
        {
            ensureSize(id);
            idToName[id] = std::string(name);
            nameToId.emplace(idToName[id], id);
        }

        /*************************************************************************************
          \brief Finds the next unused runtime component ID slot.
          \return The first free ID at or after CT_FirstRuntimeComponent.
          \throws std::runtime_error when the 16-bit ID space is exhausted.
        *************************************************************************************/
        ComponentTypeId::Storage FindNextFreeId() const
        {
            for (ComponentTypeId::Storage id = ComponentTypeId::CT_FirstRuntimeComponent;
                id < idToName.size();
                ++id)
            {
                if (idToName[id].empty())
                    return id;
            }

            if (idToName.size() >= static_cast<std::size_t>(std::numeric_limits<ComponentTypeId::Storage>::max()))
            {
                throw std::runtime_error("ComponentTypeRegistry exhausted all runtime component ids");
            }

            return static_cast<ComponentTypeId::Storage>(idToName.size());
        }

        /// Dense table indexed by component ID for reverse lookup (ID -> name).
        std::vector<std::string> idToName;
        /// Hash lookup used for forward resolution from component name to ID.
        std::unordered_map<std::string, ComponentTypeId::Storage> nameToId;
    };
}
