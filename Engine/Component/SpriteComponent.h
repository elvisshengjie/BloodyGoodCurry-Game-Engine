/*********************************************************************************************
 \file      SpriteComponent.h
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Declares the SpriteComponent class, a rendering component responsible for
            displaying textured sprites. Supports JSON-driven initialization and runtime
            resource loading through the Resource_Manager system.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once
#include <algorithm>
#include <filesystem>
#include <string_view>
#include "Composition/Component.h"
#include "Memory/ComponentPool.h"
#include "Serialization/Serialization.h"
#include "Resource_Asset_Manager/Resource_Manager.h"
#include "Component/RenderComponent.h"

namespace Framework {
    /*****************************************************************************************
      \class SpriteComponent
      \brief A rendering component that displays a sprite using a texture.

      The SpriteComponent manages a texture handle, loaded via the Resource_Manager,
      and applies it during rendering. Supports serialization to configure the texture
      key at load time, and automatically resolves or loads the texture at runtime.
      Also supports cloning for prefab duplication.
    *****************************************************************************************/
    class SpriteComponent : public GameComponent {
    public:
        // runtime
        unsigned int texture_id{ 0 };  ///< OpenGL texture ID (assigned at runtime)

        std::string texture_key; ///< Unique key used to identify the texture in Resource_Manager
        std::string path;        ///< Optional path to the texture file (used if key is missing)

        /*************************************************************************************
          \brief Initializes the sprite component by resolving its texture from Resource_Manager.

          - If a valid texture_key is set, attempts to fetch its OpenGL ID.
          - If the texture is not yet loaded but a path is provided, attempts to load the
            file and re-fetch its ID.
        *************************************************************************************/
        void initialize() override {
            if (texture_key.empty())
                return;

            texture_id = Resource_Manager::getTexture(texture_key);
            if (texture_id)
                return;

            std::string loadPath = path;

            // If SpriteComponent::path isn't provided, try to pull from RenderComponent.
            if (loadPath.empty())
            {
                if (auto* pOwner = GetOwner()) // renamed from 'owner' to avoid hiding member
                {
                    if (auto* rc = pOwner->GetComponentType<Framework::RenderComponent>(
                        ComponentTypeId::CT_RenderComponent))
                    {
                        if (!rc->texture_path.empty())
                            loadPath = rc->texture_path;
                    }
                }
            }

            if (loadPath.empty())
                return;

            std::string normalized = loadPath;
            std::replace(normalized.begin(), normalized.end(), '\\', '/');

            std::filesystem::path assetPath{ normalized };
            if (!assetPath.is_absolute())
            {
                constexpr std::string_view kLegacyPrefix = "assets/";
                constexpr std::string_view kProjectPrefix = "Assets/";

                if (const auto pos = normalized.find(kLegacyPrefix); pos != std::string::npos)
                    assetPath = normalized.substr(pos + kLegacyPrefix.size());
                else if (const auto projectPos = normalized.find(kProjectPrefix); projectPos != std::string::npos)
                    assetPath = normalized.substr(projectPos + kProjectPrefix.size());
            }

            const auto resolvedPath = assetPath.is_absolute()
                ? assetPath
                : Framework::ResolveAssetPath(assetPath);
            const std::string pathStr =
                resolvedPath.empty() ? assetPath.string() : resolvedPath.string();

            // load file and re-fetch id
            if (Resource_Manager::load(texture_key, pathStr)) {
                texture_id = Resource_Manager::getTexture(texture_key);
            }
        }

        /*************************************************************************************
          \brief Serializes the component from a JSON stream.
          \param s  Serializer reference providing the JSON data.
          \note  Reads the "texture_key" if present. The actual texture is resolved in initialize().
        *************************************************************************************/
        void Serialize(ISerializer& s) override {
            if (s.HasKey("texture_key")) StreamRead(s, "texture_key", texture_key);
            // (Optional) If you also want to read "texture_path" into SpriteComponent::path:
            // if (s.HasKey("texture_path")) StreamRead(s, "texture_path", path);
        }

        /*************************************************************************************
          \brief Creates a deep copy of the SpriteComponent.
          \return A unique_ptr to the cloned SpriteComponent.
          \note  Copies the texture_key, texture_id, and path values into the new instance.
        *************************************************************************************/
        ComponentHandle Clone() const override {
            auto copy = ComponentPool<SpriteComponent>::CreateTyped();
            copy->texture_key = texture_key;
            copy->texture_id = texture_id;
            copy->path = path;
            return copy;
        }

        /*************************************************************************************
          \brief Handles incoming messages for this component.
          \param  (unused) Currently does nothing but can be extended in the future.
        *************************************************************************************/
        void SendMessage(Message&) override {}
    };
}
