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

	GOC* GameObjectFactory::CreateEmptyComposition() {
		//Make a blank entity (new GOC()) then assigns it a unique ID
		auto* goc = new GOC();
		IdGameObject(goc);
		return goc; // Return the raw pointer to the newly created game object 
					// but the factory sill owns it because it keeps it in the GameObjectIdMap for cleanup when the game shut down
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
		//name: the string key you use to look up this component (eg Transform)
		//creator a unique_ptr<ComponentCreator> that owns the concrete creator object (eg ComponentCreatorType<Transform>)
		//ComponentMap is a std::map<std::string, std::unique_ptr<ComponentCreator>>
		//Using operator [] if name doesnt exits it creaates a new entry that key and a default constructed unique_ptr (ie nullptr) then return a reference to the value
		// if name exist it return a reference to the existing value
		//std::move (creator) transfer ownership of the unique_ptr from caller into map
		// After this the map owns the creator, the incoming creator parameter becomes null
		ComponentMap[name] = creator;

		//why this way 
		//Ownership transfer: The factory should own all registered creators so it can manage their lifetime
		//unique_ptr enforces single ownership and auto-deletes in the factory’s destructor
	}

	
}