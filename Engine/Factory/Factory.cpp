#include "Factory.h"
#include <stdexcept>
// so what the factory do?
//Create empty game object (GOCs)
//Assigns each a unique integer ID and keeps a lookup table (id -> GOCC*)
//Lets you mark GOCs for deletion, then actually delete them in update
//Hold a registry of component creators (string name -> factory object) so data driven loader can build GOCs from text or JSON file

namespace Framework {

	GameObjectFactory* FACTORY = nullptr;

	GameObjectFactory::GameObjectFactory()
	{
		if (FACTORY) throw std::runtime_error("Factory already created");
		FACTORY = this; //so FACTORY will now point to the only instance of GameObjectFactory that exist
	}

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

	GOC* GameObjectFactory::Create(const std::string& filename)
	{
		GOC* goc = BuidAndSerialize(filename);
		if (goc) goc->initialize();
		return goc;
	}

	GOC* GameObjectFactory::CreateEmptyComposition() {
		//Make a blank entity (new GOC()) then assigns it a unique ID
		auto* goc = new GOC();
		IdGameObject(goc);
		return goc; // Return the raw pointer to the newly created game object 
					// but the factory sill owns it because it keeps it in the GameObjectIdMap for cleanup when the game shut down
	}

	GOC* GameObjectFactory::BuidAndSerialize(const std::string& filename)
	{
		JsonSerializer stream;
		const bool fileOpened = stream.Open(filename);
		if (!fileOpened || !stream.IsGood())
			return nullptr;

		auto* gameObject = new GOC();

		if (!stream.EnterObject("GameObject")) {
			IdGameObject(gameObject);
			return gameObject; // empty but valid GOC
		}

		// For every registered component: if JSON has a matching key, serialize and attach it
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
			gameObject->AddComponent(creator->TypeId, std::move(comp));
		}

		stream.ExitObject();
		IdGameObject(gameObject);
		return gameObject;

	}

	void GameObjectFactory::IdGameObject(GOC* gameObject) {
		++LastGameObjectId;   //each time you create new object increment by one so every new object get a unique sequential ID  
							  //for example first object -> ID 1, second object ID -> 2
		gameObject->ObjectId = LastGameObjectId;        //since GameObjectFactory is friend class this allow to set ObjectId directly 
														//now the gameObject will know it own ID when call gameObject->getId()
		GameObjectIdMap[LastGameObjectId] = gameObject; // This line store the object pointer in the map, keyed by its unique ID
														// GameObjectIdMap[1] = pointer_to_first_object
	}

	GOC* GameObjectFactory::GetObjectWithId(GOCId id)
	{
		//Lookup by id: return GOC*(pointer) or nullptr if not found
		auto it = GameObjectIdMap.find(id);
		return it == GameObjectIdMap.end() ? nullptr : it->second; // if it does not exist it will equal to GameObjectIdMap.end()
																  
	}

	void GameObjectFactory::Destroy(GOC* gameObject)
	{
		ObjectsToBeDeleted.insert(gameObject); // Doesnt delete immediately.Just marks the object by inserting into std::set<GOC*>
											   // Using a set avoid duplicates if Destroy is called multiple times for the same GOC
	}

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

	void GameObjectFactory::AddComponentCreator(const std::string& name, ComponentCreator* creator)
	{

		ComponentMap[name] = creator;


	}

	
}