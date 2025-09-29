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