/*********************************************************************************************
 \file      Factory.h
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Declares the GameObjectFactory, which manages creation, destruction,
            and lookup of GameObjectComposition (GOC) entities. Provides facilities
            for loading prefabs from JSON, managing component creators, and ensuring
            safe object lifetime.

 \details   Responsibilities of the factory include:
            - Creating game objects (GOC) from JSON files or manually.
            - Assigning unique IDs to each object.
            - Registering component creators for dynamic construction.
            - Managing object lifetime (marking for destruction, sweeping).
            - Exposing an object map for iteration and queries.

            The factory follows a data-driven approach by storing string-to-component
            mappings, allowing JSON-defined prefabs to be built at runtime without
            hardcoding component logic.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include <map>
#include <set>
#include <memory>
#include <string>
#include "Common/System.h"
#include "Composition/Component.h"
#include "Composition/ComponentCreator.h"
#include "Composition/Composition.h"
#include "Serialization/JsonSerialization.h"

// Purpose of the factory
// - Create the gameObject aka GOC
// - Assign unique ID to them
// - Register component creators (so we can add components dynamically by name)
// - Manage object lifetime (destroying safely at the right time)
// - Load objects from data files

namespace Framework {

    /*****************************************************************************************
      \class GameObjectFactory
      \brief Central system responsible for managing all GameObjectComposition (GOC) objects.

      The factory handles:
      - Creation of GOCs (from JSON, templates, or empty).
      - Registration of components (string → ComponentCreator).
      - ID management for unique object identification.
      - Safe destruction of GOCs via deferred deletion.
      - Level loading support.

      \see GameObjectComposition, ComponentCreator, PrefabManager
    *****************************************************************************************/
    class GameObjectFactory : public ISystem {
    public:

        GameObjectFactory();
        ~GameObjectFactory() override;

        /// Create, initialize, and assign an ID to a GOC from a JSON file.
        GOC* Create(const std::string& filename);

        /// Create an empty composition (no components, new unique ID).
        GOC* CreateEmptyComposition();

        /// Create a prefab template GOC from JSON (used by PrefabManager).
        GOC* CreateTemplate(const std::string& filename);

        /// Build a GOC from the current JSON object stream.
        GOC* BuildFromCurrentJsonObject(ISerializer& stream);

        /// Build and immediately serialize a GOC from JSON file.
        GOC* BuidAndSerialize(const std::string&);

        /// Load an entire level (multiple objects) from a JSON file.
        std::vector<GOC*> CreateLevel(const std::string& filename);

        // --- Object ID & Lookup ---
        /// Assigns a unique ID to a game object.
        void IdGameObject(GOC* gameObject);

        /// Retrieves a GOC by its unique ID.
        GOC* GetObjectWithId(GOCId id);

        // --- Lifetime Management ---
        /// Marks a GOC for destruction (actual deletion deferred until Update()).
        void Destroy(GOC* gameObject);

        /// Performs deferred deletions and maintenance.
        void Update(float dt) override;

        /// Name of this system.
        std::string GetName() override { return "Factory"; }

        /// Forward messages (unused in factory).
        void SendMessage(Message* m) override { (void)m; }

        // --- Component Creator Registry ---
        /*************************************************************************************
          \brief Registers a component creator with the factory.
          \param name    The string identifier (e.g., "TransformComponent").
          \param creator Pointer to a ComponentCreator instance.
          \note The factory takes ownership and uses this for data-driven component creation.
        *************************************************************************************/
        void AddComponentCreator(const std::string& name, ComponentCreator* creator);

    private:
        unsigned LastGameObjectId = 0; ///< Counter for assigning unique GOC IDs.

        using ComponentMapType = std::map<std::string, ComponentCreator*>;
        using GameObjectIdMapType = std::map<unsigned, GOC*>;

        ComponentMapType   ComponentMap;   ///< Map: component name → ComponentCreator
        GameObjectIdMapType GameObjectIdMap; ///< Map: GOC ID → GOC pointer
        std::set<GOC*> ObjectsToBeDeleted; ///< Set of GOCs scheduled for deletion

    public:
        /// Read-only accessor for all objects managed by the factory.
        const GameObjectIdMapType& Objects() const { return GameObjectIdMap; }

        // Example iteration usage (pseudo-code):
        // void Update(float dt) override {
        //     for (auto& [id, obj] : factory_.Objects()) {
        //         if (auto* r = obj->GetComponentType<Render>(CT_Render)) {
        //             if (auto* t = obj->GetComponentType<Transform>(CT_Transform)) {
        //                 Draw(*r, *t);
        //             }
        //         }
        //     }
        // }
    };

    /// Global pointer to the active factory instance.
    extern GameObjectFactory* FACTORY;

}
