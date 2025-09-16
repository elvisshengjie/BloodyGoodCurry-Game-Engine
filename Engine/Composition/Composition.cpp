#include "Composition.h"

namespace Framework {
	//Default destructor: vector<unique_ptr<...>> automatically release own component
	GameObjectComposition::~GameObjectComposition() = default;

	void GameObjectComposition::SendMessage(Message& message) {
		for (auto& up : Components) {
			if (up) up->SendMessage(message);
		}
		
	}

	//GetComponent is member function of GameObjectComposition 
	//Its Job is to find a component attached to a game object by its ComponentTypeId and return it
	GameComponent* GameObjectComposition::GetComponent(ComponentTypeId typeId)
	{
		for (auto& up : Components) { // scan through component
			if (up && up->GetTypeId() == typeId) {
				return up.get();    // Return raw pointer(non-owner)
			}
		}
		return nullptr; // not found
	}

	GameComponent const* GameObjectComposition::GetComponent(ComponentTypeId typeId) const
	{
		for (auto const& up : Components) {
			if (up && up->GetTypeId() == typeId) {
				return up.get();
			}
		}
		return nullptr;
	}

	void GameObjectComposition::initialize() {
		for (auto& up : Components) {   // call Initialize all component
			if (up) up->Initialize();

		}
	}

	void GameObjectComposition::Destroy()
	{
		Components.clear();
	}


	// it take a new GameCompomnent(wrapped in std::unqiue_ptr for ownwership) set it up
	// and adds it to the GameObjectComposition list of components
	void GameObjectComposition::AddComponent(ComponentTypeId typeId, std::unique_ptr<GameComponent> comp)
	{
		if (!comp) return;   //ignore null
		// call the protected setter inside GameComponent to store a back-poingter to owning GameObjectComposition
		// This allow component to call GetOwner()->GetComponent<Transform>() 
		comp->set_owner(this);
		//Store the componentTypeId (e.g CT_Transform) 
		// if (comp->GetTypeId() == CT_Transform)
		comp->set_type(typeId);
		//Component is a std::vector<std::unique_ptr<GameComponent>>
		//std::move transfer ownership from caller into the vector so GameObjectComposition own it component exclusively
		//emplace_back effienctly construct or move object at the end of the vector
		Components.emplace_back(std::move(comp));
	}






}