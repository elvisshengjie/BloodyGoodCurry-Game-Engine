#pragma once
#include <map>
#include <set>
#include <memory>
#include <string>
#include "Common/System.h"
#include "Composition/Component.h"
#include "Composition/ComponentCreator.h"
#include "Composition/Composition.h"
// Purpose of the factory Create the gameObject aka GOC
// Assign unique ID to them
// Registering component creator (so we can add component dynamically by name)
// Managing object lifetime (destroying safely at the right time)
// loading object from data files

namespace Framework {
	class GameObjectFactory : public ISystem {
	public:

		GameObjectFactory();
		~GameObjectFactory() override;

		GOC* CreateEmptyComposition();

		// id & lookup
		void IdGameObject(GOC* gameObject);
		GOC* GetObjectWithId(GOCId id);

		//lifetime
		void Destroy(GOC* gameObject);
		void Update(float dt) override;
		std::string GetName() override { return "Factory";}  
		void SendMessage(Message* m) override { (void)m; }

		// creator registry (data-driven)
		void AddComponentCreator(const std::string& name, ComponentCreator* creator);

	private:
		unsigned LastGameObjectId = 0;

		using ComponentMapType = std::map<std::string, ComponentCreator*>;
		using GameObjectIdMapType = std::map<unsigned, GOC*>;

		ComponentMapType ComponentMap; // "Transform" -> creator no unique "-name-" creator is allowed and no duplicate object ID and value pairs for lookup
		GameObjectIdMapType GameObjectIdMap; // 42 => GOC*
		std::set<GOC*> ObjectsToBeDeleted;  //store only unique element and fast removal, no risk of deleting the same object


	};

	extern GameObjectFactory* FACTORY;
}

