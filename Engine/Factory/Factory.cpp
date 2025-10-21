/*********************************************************************************************
 \file      Factory.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Implements the GameObjectFactory system responsible for creating, identifying,
			serializing, and destroying GameObjectComposition (GOC) instances. Includes
			JSON-driven construction (single object and levels), prefab template creation,
			component-creator registry management, and deferred deletion.

 \details   Key behaviors:
			- Enforces a single global factory instance (FACTORY).
			- Assigns unique IDs to GOCs and maintains an id→GOC* map.
			- Supports BuildFromCurrentJsonObject for data-driven construction from an
			  already-positioned JSON serializer.
			- Defers destruction via an ObjectsToBeDeleted set to avoid mid-frame invalidation.
			- Exposes level loading by iterating a "GameObjects" JSON array.

 \copyright
			All content © 2025 DigiPen Institute of Technology Singapore.
			All rights reserved.
*********************************************************************************************/

#include "Factory.h"
#include <stdexcept>

// so what the factory do?
//Create empty game object (GOCs)
//Assigns each a unique integer ID and keeps a lookup table (id -> GOCC*)
//Lets you mark GOCs for deletion, then actually delete them in update
//Hold a registry of component creators (string name -> factory object) so data driven loader can build GOCs from text or JSON file

namespace Framework {

	/// Global singleton pointer to the active factory instance.
	GameObjectFactory* FACTORY = nullptr;

	/*************************************************************************************
	  \brief Constructs the factory and sets the global FACTORY pointer.
	  \throws std::runtime_error if a factory already exists.
	*************************************************************************************/
	GameObjectFactory::GameObjectFactory()
	{
		if (FACTORY) throw std::runtime_error("Factory already created");
		FACTORY = this; //so FACTORY will now point to the only instance of GameObjectFactory that exist
	}

	/*************************************************************************************
	  \brief Destroys the factory, cleaning up all remaining objects and creators.
	  \details
		- Deletes all GOCs still referenced by the id map.
		- Clears pending-deletion set.
		- Deletes all registered ComponentCreators.
		- Resets global FACTORY pointer to nullptr.
	*************************************************************************************/
	GameObjectFactory::~GameObjectFactory() {
		//delete all remaining objects and clear maps
		for (auto& kv : GameObjectIdMap) delete kv.second; // delete the raw pointers (kv.second) stored in map before clearing
		GameObjectIdMap.clear();
		ObjectsToBeDeleted.clear();
		//delete all registered creators
		for (auto& rc : ComponentMap) delete rc.second;
		ComponentMap.clear();
		FACTORY = nullptr;
	}

	/*************************************************************************************
	  \brief Creates a GOC from a JSON file, initializes it, and returns it.
	  \param filename Path to JSON describing a single GameObject.
	  \return Pointer to the created GOC, or nullptr on failure.
	*************************************************************************************/
	GOC* GameObjectFactory::Create(const std::string& filename)
	{
		GOC* goc = BuidAndSerialize(filename);
		if (goc) goc->initialize();
		return goc;
	}

	/*************************************************************************************
	  \brief Creates an empty GOC (no components), assigns a unique ID, and registers it.
	  \return Raw pointer to the new GOC (owned by the factory’s id map).
	*************************************************************************************/
	GOC* GameObjectFactory::CreateEmptyComposition() {
		//Make a blank entity (new GOC()) then assigns it a unique ID
		auto* goc = new GOC();
		IdGameObject(goc);
		return goc; // Return the raw pointer to the newly created game object 
		// but the factory sill owns it because it keeps it in the GameObjectIdMap for cleanup when the game shut down
	}

	/*************************************************************************************
	  \brief Creates a prefab template GOC from a JSON file without assigning an ID.
	  \param filename Path to JSON describing a single GameObject.
	  \return Pointer to the newly built GOC template (NOT tracked in id map).
	  \note Intended for PrefabManager; template is not registered with factory maps.
	*************************************************************************************/
	GOC* GameObjectFactory::CreateTemplate(const std::string& filename)
	{
		JsonSerializer s;
		if (!s.Open(filename) || !s.IsGood()) return nullptr;
		if (!s.EnterObject("GameObject")) return nullptr;

		auto* goc = new GOC();

		if (s.HasKey("name")) {
			std::string name; s.ReadString("name", name);
			goc->SetObjectName(name);
		}

		if (s.EnterObject("Components")) {
			for (auto& kv : ComponentMap) {
				const std::string& compName = kv.first;
				ComponentCreator* creator = kv.second;
				if (!s.HasKey(compName)) continue;
				if (!s.EnterObject(compName)) continue;

				std::unique_ptr<GameComponent> comp(creator->Create());
				if (comp) {
					StreamRead(s, *comp);
					goc->AddComponent(creator->TypeId, std::move(comp));
				}
				s.ExitObject();
			}
			s.ExitObject();
		}

		s.ExitObject();

		// IMPORTANT: no IdGameObject(goc); no GameObjectIdMap[...] = goc;
		return goc;
	}

	/*************************************************************************************
	  \brief Builds a GOC from the serializer’s current object (expects "Components").
	  \param stream An opened serializer, positioned at a GameObject JSON object.
	  \return Pointer to the created and ID-assigned GOC.
	  \details
		- Reads "name" if present.
		- Iterates all registered ComponentCreators and builds any present components.
		- Assigns a unique ID and registers the object in the id map.
	*************************************************************************************/
	GOC* GameObjectFactory::BuildFromCurrentJsonObject(ISerializer& stream)
	{
		auto* goc = new GOC();
		if (stream.HasKey("name")) {
			std::string name;
			stream.ReadString("name", name);
			goc->SetObjectName(name);
		}

		//Enter the Components object if present
		if (stream.EnterObject("Components")) {
			for (auto& kv : ComponentMap) {
				const std::string& compName = kv.first;
				ComponentCreator* creator = kv.second; // raw pointer OK with your current map

				if (!stream.HasKey(compName))
					continue;

				// First, try to enter the component's JSON object; only then create the component
				if (!stream.EnterObject(compName))
					continue;

				// Create as unique_ptr so any failure auto-cleans
				std::unique_ptr<GameComponent> comp(creator->Create());
				if (!comp) {
					// couldn't create; leave JSON object and continue
					stream.ExitObject();
					continue;
				}

				// Let the component load itself
				StreamRead(stream, *comp);

				// Leave the component scope
				stream.ExitObject();

				// Transfer ownership to the GOC
				goc->AddComponent(creator->TypeId, std::move(comp));
			}
			stream.ExitObject();
		}
		IdGameObject(goc);
		return goc;
	}

	/*************************************************************************************
	  \brief Opens a JSON file and builds a single GameObject if the root is "GameObject".
	  \param filename Path to JSON file.
	  \return Pointer to GOC on success (caller should call initialize()), nullptr if not a single-object file.
	  \note Use CreateLevel() for level files with arrays of objects.
	*************************************************************************************/
	GOC* GameObjectFactory::BuidAndSerialize(const std::string& filename)
	{
		JsonSerializer stream;
		if (!stream.Open(filename) || !stream.IsGood()) return nullptr;

		// Old single-object shape
		if (stream.EnterObject("GameObject")) {
			GOC* g = BuildFromCurrentJsonObject(stream);
			stream.ExitObject();
			return g;
		}

		// If the file is actually a level, just return nullptr
		// (call CreateLevel() for that file)
		return nullptr;
	}

	/*************************************************************************************
	  \brief Loads a level file containing an array "GameObjects" and builds each GOC.
	  \param filename Path to a JSON level file.
	  \return Vector of created GOC pointers (each ID-assigned and registered).
	*************************************************************************************/
	std::vector<GOC*> GameObjectFactory::CreateLevel(const std::string& filename)
	{
		JsonSerializer s;
		std::vector<GOC*> out;
		if (!s.Open(filename) || !s.IsGood()) return out;

		if (!s.EnterObject("Level")) return out;

		if (s.EnterArray("GameObjects")) {
			size_t n = s.ArraySize();
			out.reserve(n);
			for (size_t i = 0; i < n; i++) {
				if (!s.EnterIndex(i)) continue; // now at GameObjects[i]

				GOC* g = BuildFromCurrentJsonObject(s);
				if (g) { out.push_back(g); }
				s.ExitObject();                     // leave GameObject[i]
			}
			s.ExitArray();
		}
		s.ExitObject();
		return out;
	}

	/*************************************************************************************
	  \brief Assigns a unique ID to the GOC and registers it in the id→object map.
	  \param gameObject Pointer to the GOC to identify.
	*************************************************************************************/
	void GameObjectFactory::IdGameObject(GOC* gameObject) {
		++LastGameObjectId;   //each time you create new object increment by one so every new object get a unique sequential ID  
		//for example first object -> ID 1, second object ID -> 2
		gameObject->ObjectId = LastGameObjectId;        //since GameObjectFactory is friend class this allow to set ObjectId directly 
		//now the gameObject will know it own ID when call gameObject->getId()
		GameObjectIdMap[LastGameObjectId] = gameObject; // This line store the object pointer in the map, keyed by its unique ID
		// GameObjectIdMap[1] = pointer_to_first_object
	}

	/*************************************************************************************
	  \brief Looks up a GOC by its unique ID.
	  \param id The identifier to look for.
	  \return Pointer to the GOC if found, nullptr otherwise.
	*************************************************************************************/
	GOC* GameObjectFactory::GetObjectWithId(GOCId id)
	{
		//Lookup by id: return GOC*(pointer) or nullptr if not found
		auto it = GameObjectIdMap.find(id);
		return it == GameObjectIdMap.end() ? nullptr : it->second; // if it does not exist it will equal to GameObjectIdMap.end()
	}

	/*************************************************************************************
	  \brief Marks a GOC for deferred destruction.
	  \param gameObject Pointer to the object to destroy.
	  \details Inserted into a set to avoid duplicate entries and mid-frame invalidation.
	*************************************************************************************/
	void GameObjectFactory::Destroy(GOC* gameObject)
	{
		ObjectsToBeDeleted.insert(gameObject); // Doesnt delete immediately.Just marks the object by inserting into std::set<GOC*>
		// Using a set avoid duplicates if Destroy is called multiple times for the same GOC
	}


	/*************************************************************************************
	  \brief Performs the end-of-frame sweep to delete marked objects safely.
	  \param dt Delta time (unused here).
	  \details
		- Iterates the deletion set, deletes each GOC, and erases it from the id map.
		- Clears the deletion set after processing.
		- Prevents iterator invalidation / crashes during update loops.
	*************************************************************************************/
	void GameObjectFactory::Update(float dt)
	{
		dt;
		//End of frame (when update is call) actually deletes the objects and removes their entries from id map
		// The Delayed delete will prevent iterator invalidation and mid update crashes if other system are still using the object during the frame
		for (auto* obj : ObjectsToBeDeleted) {
			auto it = GameObjectIdMap.find(obj->ObjectId);
			if (it != GameObjectIdMap.end()) {
				delete obj;                // frees the GameObjectComposition
				GameObjectIdMap.erase(it); // removes the map entry (id -> pointer)
			}
		}
		ObjectsToBeDeleted.clear();  // clear the deletion set
	}

	void GameObjectFactory::Shutdown()
	{
		for (auto* obj : ObjectsToBeDeleted) {
			auto it = GameObjectIdMap.find(obj->ObjectId);
			if (it != GameObjectIdMap.end()) {
				delete obj;                // frees the GameObjectComposition
				GameObjectIdMap.erase(it); // removes the map entry (id -> pointer)
			}
		}
		ObjectsToBeDeleted.clear();  // clear the deletion set
	}

	/*************************************************************************************
	  \brief Registers a ComponentCreator under a string name for data-driven builds.
	  \param name    Component name as it appears in JSON (e.g., "TransformComponent").
	  \param creator Pointer to the ComponentCreator (factory takes ownership).
	*************************************************************************************/
	void GameObjectFactory::AddComponentCreator(const std::string& name, ComponentCreator* creator)
	{
		ComponentMap[name] = creator;
	}
}
